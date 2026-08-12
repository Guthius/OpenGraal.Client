#pragma once

#include <shared/result.hpp>

#include <filesystem>
#include <istream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace og::shared {
    enum class folder_category {
        file,
        level,
        head,
        body,
        sword,
        shield
    };

    struct folder_rule {
        folder_category category = folder_category::file;
        std::string pattern;
    };

    class settings {
      public:
        [[nodiscard]] auto contains(std::string_view key) const -> bool;
        [[nodiscard]] auto get(std::string_view key, std::string_view fallback = {}) const -> std::string;
        [[nodiscard]] auto get_int(std::string_view key, int fallback = 0) const -> int;
        [[nodiscard]] auto get_bool(std::string_view key, bool fallback = false) const -> bool;
        [[nodiscard]] auto get_list(std::string_view key) const -> std::vector<std::string>;
        [[nodiscard]] auto values() const -> const std::map<std::string, std::vector<std::string>> & { return values_; }

        void add(std::string key, std::string value);

      private:
        std::map<std::string, std::vector<std::string>> values_;
    };

    auto load_settings(const std::filesystem::path &path) -> result<settings>;
    auto load_settings(std::istream &is) -> settings;

    auto load_folder_config(const std::filesystem::path &path) -> result<std::vector<folder_rule>>;
    auto load_folder_config(std::istream &is) -> std::vector<folder_rule>;

    [[nodiscard]]
    auto parse_folder_category(std::string_view name) -> std::optional<folder_category>;

    [[nodiscard]]
    auto match_folder_pattern(std::string_view pattern, std::string_view path) -> bool;
}
