#include <catch2/catch_test_macros.hpp>

#include <server/server.hpp>

#include <filesystem>
#include <fstream>

using namespace std;
using namespace og;

namespace {
    auto write_level(const filesystem::path &path, const string_view tile, const vector<string> &links) -> void {
        ofstream board(path);

        board << "GLEVNW01\n";

        for (int y = 0; y < 64; ++y) {
            board << "BOARD 0 " << y << " 64 0 ";
            for (int x = 0; x < 64; ++x) {
                board << tile;
            }
            board << '\n';
        }

        for (const auto &link : links) {
            board << link << '\n';
        }
    }

    struct mapped_world {
        filesystem::path root = filesystem::temp_directory_path() / "opengraal-world-test";
        server::world game_world;

        mapped_world() {
            remove_all(root);
            create_directories(root / "levels");

            ofstream(root / "serveroptions.txt") << "startlevel=demo_aa-01.nw\nstartx=30\nstarty=30\n";
            ofstream(root / "folderconfig.txt") << "level *.nw\n";

            write_level(root / "levels" / "demo_aa-01.nw", "AC",
                {"LINK demo_ab-01.nw 63 0 1 64 0 playery"});
            write_level(root / "levels" / "demo_ab-01.nw", "Cs", {});

            ofstream(root / "levels" / "demo.gmap")
                << "GRMAP001\nWIDTH 2\nHEIGHT 1\nLEVELNAMES\n"
                << "\"demo_aa-01.nw\",\"demo_ab-01.nw\"\n"
                << "LEVELNAMESEND\n";
        }

        ~mapped_world() {
            remove_all(root);
        }

        mapped_world(const mapped_world &) = delete;
        auto operator=(const mapped_world &) -> mapped_world & = delete;
    };
}

TEST_CASE("a world loads its gmaps", "[world]") {
    mapped_world world;

    REQUIRE(world.game_world.load(world.root));
    REQUIRE(world.game_world.get_gmaps().size() == 1);
    REQUIRE(world.game_world.get_gmaps()[0].width == 2);
}

TEST_CASE("a level resolves to its gmap cell", "[world]") {
    mapped_world world;

    REQUIRE(world.game_world.load(world.root));

    const auto [first_map, first_cell] = world.game_world.find_gmap("demo_aa-01.nw");

    REQUIRE(first_map != nullptr);
    REQUIRE(first_cell.x == 0);
    REQUIRE(first_cell.y == 0);

    const auto [second_map, second_cell] = world.game_world.find_gmap("demo_ab-01.nw");

    REQUIRE(second_map != nullptr);
    REQUIRE(second_cell.x == 1);

    const auto [missing, ignored] = world.game_world.find_gmap("nowhere.nw");

    REQUIRE(missing == nullptr);
}

TEST_CASE("a level keeps its links", "[world]") {
    mapped_world world;

    REQUIRE(world.game_world.load(world.root));

    const auto level = world.game_world.find_level("demo_aa-01.nw");

    REQUIRE(level != nullptr);
    REQUIRE(level->links.size() == 1);
    REQUIRE(level->links[0].destination == "demo_ab-01.nw");
    REQUIRE(level->links[0].x == 63);
    REQUIRE(level->links[0].destination_y == "playery");
}

TEST_CASE("gmap neighbours are addressable by cell", "[world]") {
    mapped_world world;

    REQUIRE(world.game_world.load(world.root));

    const auto &map = world.game_world.get_gmaps()[0];

    REQUIRE(map.level_at(0, 0) == "demo_aa-01.nw");
    REQUIRE(map.level_at(1, 0) == "demo_ab-01.nw");
    REQUIRE(map.level_at(-1, 0).empty());
    REQUIRE(map.level_at(2, 0).empty());
}

TEST_CASE("a listing answers only for folderconfig categories", "[world]") {
    mapped_world world;

    ofstream(world.root / "levels" / "notes.txt") << "not listable\n";
    ofstream(world.root / "levels" / "tiles.png") << "not a level\n";

    REQUIRE(world.game_world.load(world.root));

    const auto levels = world.game_world.list_category(og::shared::folder_category::level);

    REQUIRE(levels == vector<string>{"demo_aa-01.nw", "demo_ab-01.nw"});
    REQUIRE(world.game_world.list_category(og::shared::folder_category::file).empty());
    REQUIRE(world.game_world.list_category(og::shared::folder_category::head).empty());
}

TEST_CASE("a category admits only safe names matching its patterns", "[world]") {
    mapped_world world;

    REQUIRE(world.game_world.load(world.root));

    REQUIRE(world.game_world.matches_category(og::shared::folder_category::level, "demo_aa-01.nw"));
    REQUIRE(world.game_world.matches_category(og::shared::folder_category::level, "brand-new.nw"));

    REQUIRE_FALSE(world.game_world.matches_category(og::shared::folder_category::level, ""));
    REQUIRE_FALSE(world.game_world.matches_category(og::shared::folder_category::level, "notes.txt"));
    REQUIRE_FALSE(world.game_world.matches_category(og::shared::folder_category::level, "../evil.nw"));
    REQUIRE_FALSE(world.game_world.matches_category(og::shared::folder_category::level, "sub/evil.nw"));
    REQUIRE_FALSE(world.game_world.matches_category(og::shared::folder_category::level, "sub\\evil.nw"));
    REQUIRE_FALSE(world.game_world.matches_category(og::shared::folder_category::file, "demo_aa-01.nw"));
}

TEST_CASE("replacing a level clears a cached miss and indexes a new file", "[world]") {
    mapped_world world;

    REQUIRE(world.game_world.load(world.root));
    REQUIRE(world.game_world.find_level("fresh.nw") == nullptr);

    auto fresh = make_shared<og::shared::level_data>();
    fresh->name = "fresh.nw";

    world.game_world.replace_level("fresh.nw", fresh);

    REQUIRE(world.game_world.find_level("fresh.nw") == fresh);
    REQUIRE(world.game_world.find_asset("fresh.nw") == world.root / "levels" / "fresh.nw");
    REQUIRE(world.game_world.level_file_path("fresh.nw") == world.root / "levels" / "fresh.nw");
}
