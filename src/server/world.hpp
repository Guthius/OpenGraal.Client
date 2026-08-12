#pragma once

#include <shared/config.hpp>
#include <shared/gmap.hpp>
#include <shared/level.hpp>

#include <filesystem>
#include <map>
#include <memory>
#include <string>

namespace og::server {
    struct start_position {
        std::string level;
        float x = 30.0f;
        float y = 30.0f;
    };

    class world {
      public:
        auto load(const std::filesystem::path &path) -> shared::result<bool>;

        [[nodiscard]] auto root() const -> const std::filesystem::path & { return root_; }
        [[nodiscard]] auto options() const -> const shared::settings & { return options_; }
        [[nodiscard]] auto flags() const -> const shared::settings & { return flags_; }
        [[nodiscard]] auto start() const -> const start_position & { return start_; }

        auto find_level(const std::string &name) -> std::shared_ptr<shared::level_data>;

        [[nodiscard]]
        auto find_asset(const std::string &name) const -> std::filesystem::path;

        [[nodiscard]]
        auto list_category(shared::folder_category category) const -> std::vector<std::string>;

        [[nodiscard]]
        auto matches_category(shared::folder_category category, const std::string &name) const -> bool;

        [[nodiscard]]
        auto level_file_path(const std::string &name) const -> std::filesystem::path;

        void replace_level(const std::string &name, std::shared_ptr<shared::level_data> level);

        [[nodiscard]] auto get_gmaps() const -> const std::vector<shared::gmap_data> & { return gmaps_; }
        [[nodiscard]] auto find_gmap(const std::string &level_name) const -> std::pair<const shared::gmap_data *, shared::gmap_position>;

      private:
        void index_files();
        void load_gmaps();

        std::filesystem::path root_;
        shared::settings options_;
        shared::settings flags_;
        std::vector<shared::folder_rule> folders_;
        start_position start_;

        std::vector<shared::gmap_data> gmaps_;
        std::map<std::string, std::shared_ptr<shared::level_data>> levels_;
        std::map<std::string, std::filesystem::path> files_;
    };
}
