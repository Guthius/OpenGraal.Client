#pragma once

#include <shared/result.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace og::server {
    enum class verify_result : uint8_t {
        failed,
        ok,
        needs_rehash
    };

    struct account {
        std::string name;
        std::string password_hash;
        std::string nickname;
        std::string level;
        float x = 0.0f;
        float y = 0.0f;
        uint8_t direction = 2;
        std::string head = "head0.png";
        std::string body = "body.png";
        bool banned = false;
        bool developer = false;
        std::vector<std::string> weapons;

        [[nodiscard]] auto verify(std::string_view password) const -> verify_result;
    };

    [[nodiscard]] auto hash_password(std::string_view password) -> std::string;

    [[nodiscard]] auto is_valid_email(std::string_view name) -> bool;

    auto load_account(const std::filesystem::path &path) -> shared::result<account>;
    auto save_account(const std::filesystem::path &path, const account &value) -> shared::result<bool>;
}
