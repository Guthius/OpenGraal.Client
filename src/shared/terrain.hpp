#pragma once

#include <shared/gmap.hpp>
#include <shared/tile_defs.hpp>

#include <cstdint>
#include <vector>

namespace og::shared {
    struct terrain_band {
        double ceiling;
        int block;
        int type;
    };

    struct terrain_layout {
        std::vector<terrain_band> bands;
    };

    [[nodiscard]]
    auto default_terrain_layout() -> const terrain_layout &;

    [[nodiscard]]
    auto generate_height_field(const gmap_data &map, gmap_position cell) -> std::vector<double>;

    [[nodiscard]]
    auto generate_level_tiles(const gmap_data &map, gmap_position cell, const terrain_layout &layout = default_terrain_layout()) -> std::vector<uint16_t>;

    [[nodiscard]]
    auto terrain_tile_defs(int tile_count, const terrain_layout &layout = default_terrain_layout()) -> tile_defs;
}
