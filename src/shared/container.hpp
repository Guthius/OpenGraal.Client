#pragma once

#include <shared/result.hpp>

#include <filesystem>
#include <istream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace og::shared {
    struct weapon_container {
        std::string name;
        std::string image;
        std::string script;
    };

    struct npc_container {
        std::string name;
        std::string image;
        std::string script;
        std::map<std::string, std::string> attributes;
        std::vector<std::string> flags;
        std::vector<std::string> joined_classes;

        [[nodiscard]] auto attribute(std::string_view key, std::string_view fallback = {}) const -> std::string;
        [[nodiscard]] auto attribute_int(std::string_view key, int fallback = 0) const -> int;
        [[nodiscard]] auto attribute_float(std::string_view key, float fallback = 0.0f) const -> float;
    };

    auto load_weapon(const std::filesystem::path &path) -> result<weapon_container>;
    auto load_weapon(std::istream &is) -> result<weapon_container>;

    auto load_npc(const std::filesystem::path &path) -> result<npc_container>;
    auto load_npc(std::istream &is) -> result<npc_container>;

    [[nodiscard]] auto decode_container_name(std::string_view filename) -> std::string;
    [[nodiscard]] auto encode_container_name(std::string_view name) -> std::string;
}
