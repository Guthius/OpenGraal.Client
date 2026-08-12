#include <server/world.hpp>

#include <shared/text.hpp>

#include <algorithm>
#include <format>
#include <print>

using namespace std;
using namespace og::shared;

namespace og::server {
    namespace {
        constexpr auto levels_folder = "levels";
    }

    auto world::load(const filesystem::path &path) -> result<bool> {
        if (!is_directory(path)) {
            return make_error(format("world directory not found: {}", path.string()));
        }

        root_ = path;

        if (auto options = load_settings(root_ / "serveroptions.txt")) {
            options_ = std::move(*options);
        }

        if (auto flags = load_settings(root_ / "serverflags.txt")) {
            flags_ = std::move(*flags);
        }

        if (auto folders = load_folder_config(root_ / "folderconfig.txt")) {
            folders_ = std::move(*folders);
        }

        start_ = start_position{
            .level = options_.get("startlevel"),
            .x = static_cast<float>(options_.get_int("startx", 30)),
            .y = static_cast<float>(options_.get_int("starty", 30)),
        };

        index_files();
        load_gmaps();

        if (start_.level.empty()) {
            return make_error("serveroptions.txt does not define startlevel");
        }

        if (!find_level(start_.level)) {
            return make_error(format("start level '{}' could not be loaded", start_.level));
        }

        return true;
    }

    void world::index_files() {
        files_.clear();

        const auto assets = root_ / levels_folder;
        if (!is_directory(assets)) {
            return;
        }

        for (const auto &entry : filesystem::recursive_directory_iterator(assets)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            files_.emplace(to_lower(entry.path().filename().string()), entry.path());
        }
    }

    void world::load_gmaps() {
        gmaps_.clear();

        for (const auto &[key, path] : files_) {
            if (path.extension() != ".gmap") {
                continue;
            }

            if (auto loaded = load_gmap(path)) {
                println("Loaded gmap '{}' ({}x{})", loaded->name, loaded->width, loaded->height);

                gmaps_.push_back(std::move(*loaded));
            }
        }
    }

    auto world::find_gmap(const string &level_name) const -> pair<const gmap_data *, gmap_position> {
        for (const auto &map : gmaps_) {
            if (const auto position = map.position_of(level_name)) {
                return {&map, *position};
            }
        }

        return {nullptr, gmap_position{}};
    }

    auto world::find_level(const string &name) -> shared_ptr<level_data> {
        const auto key = to_lower(name);

        if (const auto cached = levels_.find(key); cached != levels_.end()) {
            return cached->second;
        }

        const auto path = find_asset(name);
        if (path.empty()) {
            levels_[key] = nullptr;

            return nullptr;
        }

        auto loaded = load_level(path);
        if (!loaded) {
            levels_[key] = nullptr;

            return nullptr;
        }

        auto level = make_shared<level_data>(std::move(*loaded));
        levels_[key] = level;

        return level;
    }

    auto world::find_asset(const string &name) const -> filesystem::path {
        if (name.empty() || name.contains("..")) {
            return {};
        }

        const auto match = files_.find(to_lower(filesystem::path(name).filename().string()));

        return match == files_.end() ? filesystem::path{} : match->second;
    }

    auto world::list_category(const folder_category category) const -> vector<string> {
        vector<string> names;

        for (const auto &[key, path] : files_) {
            const auto relative = path.lexically_relative(root_ / levels_folder).generic_string();

            for (const auto &rule : folders_) {
                if (rule.category == category && match_folder_pattern(rule.pattern, relative)) {
                    names.push_back(path.filename().string());
                    break;
                }
            }
        }

        return names;
    }

    auto world::matches_category(const folder_category category, const string &name) const -> bool {
        if (name.empty() || name.contains("..") || name.contains('/') || name.contains('\\')) {
            return false;
        }

        auto relative = name;
        if (const auto path = find_asset(name); !path.empty()) {
            relative = path.lexically_relative(root_ / levels_folder).generic_string();
        }

        return ranges::any_of(folders_, [&](const folder_rule &rule) {
            return rule.category == category && match_folder_pattern(rule.pattern, relative);
        });
    }

    auto world::level_file_path(const string &name) const -> filesystem::path {
        if (const auto path = find_asset(name); !path.empty()) {
            return path;
        }

        return root_ / levels_folder / name;
    }

    void world::replace_level(const string &name, shared_ptr<level_data> level) {
        const auto key = to_lower(name);

        levels_[key] = std::move(level);
        files_.try_emplace(key, root_ / levels_folder / name);
    }
}
