#include <catch2/catch_test_macros.hpp>

#include <shared/tile_defs.hpp>

#include <filesystem>

using namespace std;
using namespace og::shared;

namespace {
    constexpr int pics1_tile_count = 4096;
}

TEST_CASE("a format with no known layout blocks nothing", "[tiledefs]") {
    const auto defs = tile_defs_for_format(99, pics1_tile_count);

    REQUIRE(defs.empty());
    REQUIRE(defs.type_of(0) == tile_passable);
    REQUIRE(defs.type_of(2066) == tile_passable);
}

TEST_CASE("a terrain format types the tiles the generator places, and nothing else", "[tiledefs]") {
    const auto defs = tile_defs_for_format(tileset_format_terrain, pics1_tile_count);

    REQUIRE_FALSE(defs.empty());

    // The sea tile the generator fills its lowest band with, and the beach above it.
    REQUIRE(defs.type_of(34) == tile_water);
    REQUIRE(defs.type_of(42) == tile_near_water);

    // Grass and rock are walked on, and the rest of the sheet is not described at all.
    REQUIRE(defs.type_of(1058) == tile_passable);
    REQUIRE(defs.type_of(1570) == tile_passable);
    REQUIRE(defs.type_of(700) == tile_passable);
}

TEST_CASE("format 1 types the tiles the sample world's houses are built from", "[tiledefs]") {
    const auto defs = tile_defs_for_format(tileset_format_new_world, pics1_tile_count);

    REQUIRE_FALSE(defs.empty());

    // Floor: the tiles Thamhic's inn is carpeted with, and the ones it walls that room with.
    REQUIRE(defs.type_of(0) == tile_passable);
    REQUIRE(defs.type_of(32) == tile_passable);
    REQUIRE(defs.type_of(2066) == tile_passable);
    REQUIRE(defs.type_of(2067) == tile_passable);

    REQUIRE(defs.type_of(1350) == tile_wall);
    REQUIRE(defs.type_of(1387) == tile_wall);
    REQUIRE(defs.type_of(1403) == tile_wall);
}

TEST_CASE("format 1 places water, chairs and beds where the template does", "[tiledefs]") {
    const auto defs = tile_defs_for_format(tileset_format_new_world, pics1_tile_count);

    REQUIRE(defs.type_of(64) == tile_water);       // block 0, rows 4-11
    REQUIRE(defs.type_of(320) == tile_chair);      // block 0, rows 20-23
    REQUIRE(defs.type_of(384) == tile_bed_top);    // block 0, row 24
    REQUIRE(defs.type_of(400) == tile_bed_bottom); // block 0, row 25
    REQUIRE(defs.type_of(496) == tile_jump);       // block 0, rows 30-31
    REQUIRE(defs.type_of(512) == tile_wall);       // block 1 opens with 16 blocking rows
    REQUIRE(defs.type_of(832) == tile_near_water); // block 1, rows 20-24
}

TEST_CASE("the type of a tile outside the tileset is passable", "[tiledefs]") {
    const auto defs = tile_defs_for_format(tileset_format_new_world, 512);

    REQUIRE(defs.type_of(-1) == tile_passable);
    REQUIRE(defs.type_of(512) == tile_passable);
}

TEST_CASE("arrays.dat types the pics1 tiles the offline level is built from", "[tiledefs]") {
    const auto defs = load_tile_defs(std::filesystem::path(OPENGRAAL_ASSET_PATH) / "arrays.dat", pics1_tile_count);

    REQUIRE(defs);
    REQUIRE_FALSE(defs->empty());

    auto count_of = [&defs](const int type) {
        auto total = 0;
        for (auto id = 0; id < pics1_tile_count; ++id) {
            total += defs->type_of(id) == type ? 1 : 0;
        }

        return total;
    };

    INFO("water " << count_of(tile_water) << " chair " << count_of(tile_chair) << " near " << count_of(tile_near_water));
    REQUIRE(count_of(tile_water) > 0);
    REQUIRE(count_of(tile_chair) > 0);
}
