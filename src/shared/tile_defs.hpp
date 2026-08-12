#pragma once

#include <shared/result.hpp>

#include <filesystem>
#include <istream>
#include <vector>

namespace og::shared {
    enum tile_type : int {
        tile_passable = 0,
        tile_wall = 1 << 0,
        tile_water = 1 << 1,
        tile_chair = 1 << 2,
        tile_near_water = 1 << 3,
        tile_swamp = 1 << 4,
        tile_jump = 1 << 5,
        tile_bed_top = 1 << 6,
        tile_bed_bottom = 1 << 7,
    };

    class tile_defs {
      public:
        tile_defs() = default;

        explicit tile_defs(std::vector<int> types);

        [[nodiscard]] auto type_of(int id) const -> int;
        [[nodiscard]] auto size() const -> size_t { return types_.size(); }
        [[nodiscard]] auto empty() const -> bool { return types_.empty(); }

      private:
        std::vector<int> types_;
    };

    auto load_tile_defs(const std::filesystem::path &path, int tile_count) -> result<tile_defs>;
    auto load_tile_defs(std::istream &is, int tile_count) -> result<tile_defs>;

    enum tileset_format : int {
        tileset_format_pics1 = 0,
        tileset_format_new_world = 1,
        tileset_format_terrain = 5,
    };

    [[nodiscard]] auto tile_defs_for_format(int format, int tile_count) -> tile_defs;
}
