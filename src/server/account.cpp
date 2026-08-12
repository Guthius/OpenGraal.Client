#include <server/account.hpp>

#include <shared/container.hpp>
#include <shared/random.hpp>
#include <shared/text.hpp>

#include <argon2.h>

#include <algorithm>
#include <cstring>
#include <format>
#include <fstream>

using namespace std;
using namespace og::shared;

namespace og::server {
    namespace {
        constexpr string_view signature = "OGACC001";
        constexpr uint64_t hash_offset = 1469598103934665603ull;
        constexpr uint64_t hash_prime = 1099511628211ull;

        constexpr uint32_t argon2_time_cost = 3;
        constexpr uint32_t argon2_memory_cost = 65536;
        constexpr uint32_t argon2_parallelism = 1;
        constexpr size_t argon2_hash_length = 32;
        constexpr size_t argon2_salt_length = 16;
        constexpr size_t legacy_hash_length = 16;
        constexpr size_t max_local_part = 64;
        constexpr size_t min_email_length = 3;
        constexpr size_t max_email_length = 254;

        auto split_key(const string_view line) -> pair<string, string> {
            const auto separator = line.find(' ');
            if (separator == string_view::npos) {
                return {string(line), {}};
            }

            return {string(line.substr(0, separator)), trim(line.substr(separator + 1))};
        }

        auto legacy_hash_password(const string_view password) -> string {
            auto hash = hash_offset;
            for (const auto ch : password) {
                hash ^= static_cast<uint8_t>(ch);
                hash *= hash_prime;
            }

            return format("{:016x}", hash);
        }

        auto is_legacy_hash(const string_view hash) -> bool {
            return hash.size() == legacy_hash_length &&
                   std::ranges::all_of(hash, [](const unsigned char ch) { return isxdigit(ch) != 0; });
        }

        auto is_valid_local_part(const string_view part) -> bool {
            if (part.empty() || part.size() > max_local_part || part.starts_with('.') || part.ends_with('.')) {
                return false;
            }

            if (part.find("..") != string_view::npos) {
                return false;
            }

            return std::ranges::all_of(part, [](const unsigned char ch) {
                return isalnum(ch) != 0 || ch == '.' || ch == '_' || ch == '-' || ch == '+';
            });
        }

        auto is_valid_domain(const string_view domain) -> bool {
            if (domain.empty() || domain.starts_with('.') || domain.ends_with('.') ||
                domain.starts_with('-') || domain.ends_with('-')) {
                return false;
            }

            if (domain.find("..") != string_view::npos || domain.find('.') == string_view::npos) {
                return false;
            }

            return std::ranges::all_of(domain, [](const unsigned char ch) {
                return isalnum(ch) != 0 || ch == '.' || ch == '-';
            });
        }
    }

    auto hash_password(const string_view password) -> string {
        const auto salt = random_bytes(argon2_salt_length);

        string encoded(argon2_encodedlen(argon2_time_cost, argon2_memory_cost, argon2_parallelism,
                           static_cast<uint32_t>(salt.size()), argon2_hash_length, Argon2_id),
            '\0');

        const auto code = argon2id_hash_encoded(argon2_time_cost, argon2_memory_cost, argon2_parallelism,
            password.data(), password.size(), salt.data(), salt.size(),
            argon2_hash_length, encoded.data(), encoded.size());

        if (code != ARGON2_OK) {
            return {};
        }

        encoded.resize(strlen(encoded.c_str()));

        return encoded;
    }

    auto is_valid_email(const string_view name) -> bool {
        if (name.size() < min_email_length || name.size() > max_email_length) {
            return false;
        }

        const auto at = name.find('@');
        if (at == string_view::npos || name.find('@', at + 1) != string_view::npos) {
            return false;
        }

        return is_valid_local_part(name.substr(0, at)) && is_valid_domain(name.substr(at + 1));
    }

    auto account::verify(const string_view password) const -> verify_result {
        if (password_hash.empty()) {
            return verify_result::failed;
        }

        if (is_legacy_hash(password_hash)) {
            return equals_constant_time(legacy_hash_password(password), password_hash)
                       ? verify_result::needs_rehash
                       : verify_result::failed;
        }

        const auto code = argon2id_verify(password_hash.c_str(), password.data(), password.size());

        return code == ARGON2_OK ? verify_result::ok : verify_result::failed;
    }

    auto load_account(const filesystem::path &path) -> result<account> {
        ifstream ifs(path);
        if (!ifs) {
            return make_error(format("could not open '{}'", path.string()));
        }

        string line;
        if (!getline(ifs, line) || trim(line) != signature) {
            return make_error(format("'{}' is not an account file", path.string()));
        }

        account value;
        while (getline(ifs, line)) {
            const auto [key, text] = split_key(trim(line));

            if (key == "NAME")
                value.name = text;
            else if (key == "PASSWORD")
                value.password_hash = text;
            else if (key == "NICK")
                value.nickname = text;
            else if (key == "LEVEL")
                value.level = text;
            else if (key == "X")
                value.x = to_float(text);
            else if (key == "Y")
                value.y = to_float(text);
            else if (key == "DIR")
                value.direction = static_cast<uint8_t>(to_int(text));
            else if (key == "HEAD")
                value.head = text;
            else if (key == "BODY")
                value.body = text;
            else if (key == "BANNED")
                value.banned = to_bool(text);
            else if (key == "DEVELOPER")
                value.developer = to_bool(text);
            else if (key == "WEAPON" && !text.empty())
                value.weapons.push_back(text);
        }

        if (value.name.empty()) {
            value.name = decode_container_name(path.stem().string());
        }

        return value;
    }

    auto save_account(const filesystem::path &path, const account &value) -> result<bool> {
        error_code code;
        create_directories(path.parent_path(), code);

        ofstream ofs(path, ios::trunc);
        if (!ofs) {
            return make_error(format("could not write '{}'", path.string()));
        }

        ofs << signature << '\n'
            << "NAME " << value.name << '\n'
            << "PASSWORD " << value.password_hash << '\n'
            << "NICK " << value.nickname << '\n'
            << "LEVEL " << value.level << '\n'
            << format("X {:.2f}\n", value.x)
            << format("Y {:.2f}\n", value.y)
            << "DIR " << static_cast<int>(value.direction) << '\n'
            << "HEAD " << value.head << '\n'
            << "BODY " << value.body << '\n'
            << "BANNED " << (value.banned ? "true" : "false") << '\n'
            << "DEVELOPER " << (value.developer ? "true" : "false") << '\n';

        for (const auto &weapon : value.weapons) {
            ofs << "WEAPON " << weapon << '\n';
        }

        return true;
    }
}
