#include "token.hpp"

#include "random.hpp"

#include <blake2/blake2.h>

#include <array>
#include <charconv>
#include <chrono>
#include <format>

namespace og::shared {
    namespace {
        constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        constexpr std::string_view header_json = R"({"alg":"BLAKE2B256","typ":"JWT"})";
        constexpr std::size_t signature_bytes = 32;
        constexpr std::size_t key_bytes = 64;
        constexpr std::size_t min_secret_length = 32;
        constexpr std::int64_t max_clock_skew = 60;

        auto decode_digit(const char ch) -> int {
            const auto found = alphabet.find(ch);

            return found == std::string_view::npos ? -1 : static_cast<int>(found);
        }

        auto sign(const std::string_view message, const std::string_view secret) -> std::string {
            std::array<std::uint8_t, key_bytes> key{};
            if (blake2b(key.data(), key.size(), secret.data(), secret.size(), nullptr, 0) != 0) {
                return {};
            }

            std::array<std::uint8_t, signature_bytes> digest{};
            if (blake2b(digest.data(), digest.size(), message.data(), message.size(), key.data(), key.size()) != 0) {
                return {};
            }

            return {reinterpret_cast<const char *>(digest.data()), digest.size()};
        }

        auto find_string_field(const std::string_view json, const std::string_view key) -> std::optional<std::string> {
            const auto needle = std::format("\"{}\":\"", key);

            const auto start = json.find(needle);
            if (start == std::string_view::npos) {
                return std::nullopt;
            }

            const auto from = start + needle.size();

            const auto end = json.find('"', from);
            if (end == std::string_view::npos) {
                return std::nullopt;
            }

            const auto value = json.substr(from, end - from);

            return !value.contains('\\') ? std::optional{std::string(value)} : std::nullopt;
        }

        auto find_int_field(const std::string_view json, const std::string_view key) -> std::optional<std::int64_t> {
            const auto needle = std::format("\"{}\":", key);

            const auto start = json.find(needle);
            if (start == std::string_view::npos) {
                return std::nullopt;
            }

            const auto *const from = json.data() + start + needle.size();
            const auto *const last = json.data() + json.size();

            std::int64_t value = 0;
            if (const auto [ptr, code] = std::from_chars(from, last, value); code != std::errc{}) {
                return std::nullopt;
            }

            return value;
        }
    }

    auto base64url_encode(const std::string_view data) -> std::string {
        std::string encoded;
        encoded.reserve((data.size() + 2) / 3 * 4);

        for (std::size_t i = 0; i < data.size(); i += 3) {
            const auto remaining = data.size() - i;

            const auto b0 = static_cast<unsigned char>(data[i]);
            const auto b1 = remaining > 1 ? static_cast<unsigned char>(data[i + 1]) : 0;
            const auto b2 = remaining > 2 ? static_cast<unsigned char>(data[i + 2]) : 0;

            encoded += alphabet[b0 >> 2];
            encoded += alphabet[((b0 & 0x03) << 4) | (b1 >> 4)];

            if (remaining > 1) {
                encoded += alphabet[((b1 & 0x0f) << 2) | (b2 >> 6)];
            }

            if (remaining > 2) {
                encoded += alphabet[b2 & 0x3f];
            }
        }

        return encoded;
    }

    auto base64url_decode(const std::string_view text) -> std::optional<std::string> {
        if (text.size() % 4 == 1) {
            return std::nullopt;
        }

        std::string decoded;
        decoded.reserve(text.size() / 4 * 3);

        for (std::size_t i = 0; i < text.size(); i += 4) {
            const auto remaining = text.size() - i;

            std::array<int, 4> group{0, 0, 0, 0};
            for (std::size_t j = 0; j < 4; ++j) {
                if (j >= remaining) {
                    continue;
                }

                group[j] = decode_digit(text[i + j]);
                if (group[j] < 0) {
                    return std::nullopt;
                }
            }

            decoded += static_cast<char>((group[0] << 2) | (group[1] >> 4));

            if (remaining > 2) {
                decoded += static_cast<char>(((group[1] & 0x0f) << 4) | (group[2] >> 2));
            }

            if (remaining > 3) {
                decoded += static_cast<char>(((group[2] & 0x03) << 6) | group[3]);
            }
        }

        return decoded;
    }

    auto unix_now() -> std::int64_t {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    auto issue_token(const token_claims &claims, const std::string_view secret) -> std::string {
        if (secret.size() < min_secret_length || claims.account.empty()) {
            return {};
        }

        if (claims.account.contains('"') || claims.account.contains('\\')) {
            return {};
        }

        const auto payload = std::format(R"({{"sub":"{}","iat":{},"exp":{}}})", claims.account, claims.issued_at, claims.expires_at);
        const auto message = std::format("{}.{}", base64url_encode(header_json), base64url_encode(payload));

        const auto signature = sign(message, secret);
        if (signature.empty()) {
            return {};
        }

        return std::format("{}.{}", message, base64url_encode(signature));
    }

    auto verify_token(const std::string_view token, const std::string_view secret, const std::int64_t now) -> std::optional<token_claims> {
        if (secret.size() < min_secret_length) {
            return std::nullopt;
        }

        const auto first = token.find('.');
        if (first == std::string_view::npos) {
            return std::nullopt;
        }

        const auto second = token.find('.', first + 1);
        if (second == std::string_view::npos || token.find('.', second + 1) != std::string_view::npos) {
            return std::nullopt;
        }

        const auto expected = sign(token.substr(0, second), secret);
        if (expected.empty()) {
            return std::nullopt;
        }

        const auto signature = base64url_decode(token.substr(second + 1));
        if (!signature || !equals_constant_time(*signature, expected)) {
            return std::nullopt;
        }

        const auto header = base64url_decode(token.substr(0, first));
        if (!header || find_string_field(*header, "alg") != "BLAKE2B256") {
            return std::nullopt;
        }

        const auto payload = base64url_decode(token.substr(first + 1, second - first - 1));
        if (!payload) {
            return std::nullopt;
        }

        const auto account = find_string_field(*payload, "sub");
        const auto issued_at = find_int_field(*payload, "iat");
        const auto expires_at = find_int_field(*payload, "exp");

        if (!account || account->empty() || !issued_at || !expires_at) {
            return std::nullopt;
        }

        if (*expires_at <= now || *issued_at > now + max_clock_skew) {
            return std::nullopt;
        }

        return token_claims{.account = *account, .issued_at = *issued_at, .expires_at = *expires_at};
    }
}
