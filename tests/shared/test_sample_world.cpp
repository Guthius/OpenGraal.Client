#include <catch2/catch_test_macros.hpp>

#include <shared/config.hpp>
#include <shared/container.hpp>
#include <shared/gani.hpp>
#include <shared/gmap.hpp>
#include <shared/level.hpp>

#include <filesystem>
#include <format>
#include <string>
#include <vector>

using namespace std;
using namespace og::shared;

namespace {
    const auto test_world = filesystem::path(OPENGRAAL_TEST_WORLD_PATH);

    auto files_with_extension(const filesystem::path &directory, const string_view extension) -> vector<filesystem::path> {
        vector<filesystem::path> paths;

        if (!is_directory(directory)) {
            return paths;
        }

        for (const auto &entry : filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == extension) {
                paths.push_back(entry.path());
            }
        }

        return paths;
    }

    template <typename TLoader>
    void require_all_parse(const vector<filesystem::path> &paths, TLoader loader) {
        vector<string> failures;

        for (const auto &path : paths) {
            if (auto parsed = loader(path); !parsed) {
                failures.push_back(format("{}: {}", path.filename().string(), parsed.error()));
            }
        }

        INFO(format("{} of {} failed:\n{}", failures.size(), paths.size(), [&failures] {
            string joined;
            for (const auto &failure : failures) {
                joined += "  " + failure + "\n";
            }
            return joined;
        }()));

        REQUIRE(failures.empty());
    }
}

TEST_CASE("every level in the test world parses", "[world]") {
    const auto paths = files_with_extension(test_world / "levels", ".nw");
    if (paths.empty()) {
        SKIP(format("no levels under {}", (test_world / "levels").string()));
    }

    require_all_parse(paths, [](const auto &path) { return load_level(path); });
}

TEST_CASE("every gmap in the test world parses and covers its grid", "[world]") {
    const auto paths = files_with_extension(test_world / "levels", ".gmap");
    if (paths.empty()) {
        SKIP("no gmaps in the test world");
    }

    require_all_parse(paths, [](const auto &path) { return load_gmap(path); });

    for (const auto &path : paths) {
        const auto gmap = load_gmap(path);

        INFO(path.filename().string());
        REQUIRE(gmap->width > 0);
        REQUIRE(gmap->height > 0);
        REQUIRE(gmap->level_names.size() == static_cast<size_t>(gmap->width) * static_cast<size_t>(gmap->height));
    }
}

TEST_CASE("every gani in the test world parses", "[world]") {
    const auto paths = files_with_extension(test_world / "levels" / "ganis", ".gani");
    if (paths.empty()) {
        SKIP("no ganis in the test world");
    }

    require_all_parse(paths, [](const auto &path) { return load_gani(path); });
}

TEST_CASE("every weapon container in the test world parses", "[world]") {
    const auto paths = files_with_extension(test_world / "weapons", ".txt");
    if (paths.empty()) {
        SKIP("no weapons in the test world");
    }

    require_all_parse(paths, [](const auto &path) { return load_weapon(path); });

    for (const auto &path : paths) {
        const auto weapon = load_weapon(path);

        INFO(path.filename().string());
        REQUIRE_FALSE(weapon->name.empty());
        REQUIRE_FALSE(weapon->script.empty());
    }
}

TEST_CASE("every npc container in the test world parses", "[world]") {
    const auto paths = files_with_extension(test_world / "npcs", ".txt");
    if (paths.empty()) {
        SKIP("no npcs in the test world");
    }

    require_all_parse(paths, [](const auto &path) { return load_npc(path); });

    for (const auto &path : paths) {
        const auto npc = load_npc(path);

        INFO(path.filename().string());
        REQUIRE_FALSE(npc->name.empty());
        REQUIRE(npc->attributes.contains("ID"));
    }
}

TEST_CASE("the test world configuration files parse", "[world]") {
    if (!is_directory(test_world)) {
        SKIP(format("no test world at {}", test_world.string()));
    }

    const auto options = load_settings(test_world / "serveroptions.txt");

    REQUIRE(options);
    REQUIRE(options->get("startlevel") == "thamhicinside_house_start.nw");
    REQUIRE(options->get_int("startx") == 23);
    REQUIRE(options->get_bool("putnpcenabled") == false);
    REQUIRE(options->get_list("gmaps") == vector<string>{"thamhic"});

    const auto folders = load_folder_config(test_world / "folderconfig.txt");

    REQUIRE(folders);
    REQUIRE_FALSE(folders->empty());

    const auto flags = load_settings(test_world / "serverflags.txt");

    REQUIRE(flags);
    REQUIRE(flags->get("server.start_level") == "thamhicinside_house_start.nw");
}
