#include <catch2/catch_test_macros.hpp>

#include <shared/level.hpp>

#include <filesystem>
#include <sstream>

using namespace std;
using namespace og::shared;

TEST_CASE("an unknown signature is rejected", "[level]") {
    istringstream is("NOTALEVEL\n");

    REQUIRE_FALSE(load_level("bad.nw", is));
}

TEST_CASE("nw boards decode into the requested layer", "[level]") {
    istringstream is(
        "GLEVNW01\n"
        "BOARD 0 0 2 0 ACAD\n"
        "BOARD 0 1 1 1 BA\n");

    const auto level = load_level("board.nw", is);

    REQUIRE(level);
    REQUIRE(level->layers.size() == 2);

    const auto *base = level->tiles();

    REQUIRE(base != nullptr);
    REQUIRE(base->size() == level_tile_count);
    REQUIRE((*base)[0] == ((0 << 6) | 2));
    REQUIRE((*base)[1] == ((0 << 6) | 3));

    const auto *overlay = level->find_layer(1);

    REQUIRE(overlay != nullptr);
    REQUIRE(overlay->tiles[level_width] == ((1 << 6) | 0));
}

TEST_CASE("nw links keep destination names containing spaces", "[level]") {
    istringstream is(
        "GLEVNW01\n"
        "LINK my level.nw 30 31 2 3 playerx playery\n");

    const auto level = load_level("links.nw", is);

    REQUIRE(level);
    REQUIRE(level->links.size() == 1);

    const auto &link = level->links.front();

    REQUIRE(link.destination == "my level.nw");
    REQUIRE(link.x == 30);
    REQUIRE(link.y == 31);
    REQUIRE(link.width == 2);
    REQUIRE(link.height == 3);
    REQUIRE(link.destination_x == "playerx");
    REQUIRE(link.destination_y == "playery");
}

TEST_CASE("nw signs, npcs, chests and baddies are captured", "[level]") {
    istringstream is(
        "GLEVNW01\n"
        "SIGN 10 11\n"
        "hello\n"
        "SIGNEND\n"
        "NPC - 41 19\n"
        "join nopking;\n"
        "NPCEND\n"
        "CHEST 5 6 greenrupee 0\n"
        "BADDY 7 8 3\n"
        "verse one\n"
        "BADDYEND\n");

    const auto level = load_level("stuff.nw", is);

    REQUIRE(level);

    REQUIRE(level->signs.size() == 1);
    REQUIRE(level->signs[0].x == 10);
    REQUIRE(level->signs[0].text == "hello\n");

    REQUIRE(level->npcs.size() == 1);
    REQUIRE(level->npcs[0].image.empty());
    REQUIRE(level->npcs[0].x == 41.0f);
    REQUIRE(level->npcs[0].y == 19.0f);
    REQUIRE(level->npcs[0].script == "join nopking;\n");

    REQUIRE(level->chests.size() == 1);
    REQUIRE(level->chests[0].item == "greenrupee");

    REQUIRE(level->baddies.size() == 1);
    REQUIRE(level->baddies[0].type == 3);
    REQUIRE(level->baddies[0].verses.size() == 1);
}

TEST_CASE("npc images may contain spaces", "[level]") {
    istringstream is(
        "GLEVNW01\n"
        "NPC my npc.png 4 5\n"
        "NPCEND\n");

    const auto level = load_level("npc.nw", is);

    REQUIRE(level);
    REQUIRE(level->npcs.size() == 1);
    REQUIRE(level->npcs[0].image == "my npc.png");
    REQUIRE(level->npcs[0].x == 4.0f);
}

TEST_CASE("the bundled starter level loads", "[level]") {
    const auto path = filesystem::path(OPENGRAAL_ASSET_PATH) / "levels" / "onlinestartlocal.nw";
    if (!exists(path)) {
        SKIP("starter level not present");
    }

    const auto level = load_level(path);

    REQUIRE(level);
    REQUIRE(level->version == "GLEVNW01");
    REQUIRE(level->tiles() != nullptr);
}
