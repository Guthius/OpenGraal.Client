#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace og::shared {
    struct token_claims {
        std::string account;
        std::int64_t issued_at = 0;
        std::int64_t expires_at = 0;
    };

    [[nodiscard]] auto issue_token(const token_claims &claims, std::string_view secret) -> std::string;

    [[nodiscard]] auto verify_token(std::string_view token, std::string_view secret, std::int64_t now) -> std::optional<token_claims>;

    [[nodiscard]] auto base64url_encode(std::string_view data) -> std::string;
    [[nodiscard]] auto base64url_decode(std::string_view text) -> std::optional<std::string>;

    [[nodiscard]] auto unix_now() -> std::int64_t;
}
