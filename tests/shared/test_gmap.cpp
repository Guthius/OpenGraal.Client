#include <catch2/catch_test_macros.hpp>

#include <catch2/catch_approx.hpp>

#include <shared/gmap.hpp>
#include <shared/terrain.hpp>

#include <sstream>

using namespace std;
using namespace og::shared;

TEST_CASE("gmap requires the GRMAP001 signature", "[gmap]") {
    istringstream is("NOTAGMAP\nWIDTH 4\n");

    REQUIRE_FALSE(load_gmap("bad.gmap", is));
}

TEST_CASE("gmap reads an explicit level name grid", "[gmap]") {
    istringstream is(
        "GRMAP001\n"
        "WIDTH 2\n"
        "HEIGHT 2\n"
        "LEVELNAMES\n"
        "\"a1.nw\",\"b1.nw\"\n"
        "\"a2.nw\",\"b2.nw\"\n"
        "LEVELNAMESEND\n"
        "MAPIMG world.png\n");

    const auto gmap = load_gmap("world.gmap", is);

    REQUIRE(gmap);
    REQUIRE(gmap->width == 2);
    REQUIRE(gmap->height == 2);
    REQUIRE(gmap->map_image == "world.png");
    REQUIRE(gmap->level_at(0, 0) == "a1.nw");
    REQUIRE(gmap->level_at(1, 1) == "b2.nw");
    REQUIRE(gmap->level_at(9, 9).empty());

    const auto position = gmap->position_of("B2.NW");

    REQUIRE(position);
    REQUIRE(position->x == 1);
    REQUIRE(position->y == 1);
}

TEST_CASE("generated level names follow the two-letter column scheme", "[gmap]") {
    const auto names = derive_generated_level_names("thamhic_bf-32.nw", 32, 32);

    REQUIRE(names.size() == 32 * 32);
    REQUIRE(names.front() == "thamhic_aa-01.nw");
    REQUIRE(names[25] == "thamhic_az-01.nw");
    REQUIRE(names[26] == "thamhic_ba-01.nw");
    REQUIRE(names.back() == "thamhic_bf-32.nw");
}

TEST_CASE("a gmap without LEVELNAMES derives them from GENERATED", "[gmap]") {
    istringstream is(
        "GRMAP001\n"
        "WIDTH 32\n"
        "HEIGHT 32\n"
        "GENERATED thamhic_bf-32.nw\n");

    const auto gmap = load_gmap("thamhic.gmap", is);

    REQUIRE(gmap);
    REQUIRE(gmap->level_names.size() == 32 * 32);
    REQUIRE(gmap->level_at(0, 0) == "thamhic_aa-01.nw");
    REQUIRE(gmap->level_at(31, 31) == "thamhic_bf-32.nw");
}

TEST_CASE("a generated gmap builds tiles from its heightmap", "[gmap]") {
    const auto source = string(
        "GRMAP001\n"
        "WIDTH 2\n"
        "HEIGHT 2\n"
        "GENERATED world_aa-01.nw\n"
        "LEVHEIGHT 4\n"
        "LEVCHAOS 0.5\n"
        "HEIGHTMAP\n"
        "-40,-40,-40\n"
        "-40,90,90\n"
        "-40,90,90\n"
        "HEIGHTMAPEND\n"
        "RANDOMSEEDS\n"
        "11,22\n"
        "33,44\n"
        "RANDOMSEEDSEND\n");

    istringstream is(source);
    const auto map = load_gmap("world.gmap", is);

    REQUIRE(map);
    REQUIRE(map->height_map.size() == 9);
    REQUIRE(map->random_seeds == vector<uint32_t>{11, 22, 33, 44});
    REQUIRE(map->level_height == 4.0);

    // The corner heights are pinned, so the low corner is sea and the high one is not.
    const auto field = generate_height_field(*map, {.x = 0, .y = 0});

    REQUIRE(field.size() == 64 * 64);
    REQUIRE(field.front() < 0.0);
    REQUIRE(field.back() > 0.0);

    const auto tiles = generate_level_tiles(*map, {.x = 0, .y = 0});

    REQUIRE(tiles.size() == 64 * 64);
    REQUIRE(tiles.front() != tiles.back());

    // Neighbours must agree on the edge they share, or the world has seams down every border.
    const auto left = generate_height_field(*map, {.x = 0, .y = 0});
    const auto right = generate_height_field(*map, {.x = 1, .y = 0});

    REQUIRE(left[(32 * 64) + 63] == Catch::Approx(right[32 * 64]).margin(1.0));
}

TEST_CASE("a gmap with no heightmap generates nothing", "[gmap]") {
    const auto source = string("GRMAP001\nWIDTH 2\nHEIGHT 2\nGENERATED world_aa-01.nw\n");

    istringstream is(source);
    const auto map = load_gmap("world.gmap", is);

    REQUIRE(map);
    REQUIRE(generate_level_tiles(*map, {.x = 0, .y = 0}).empty());
}
