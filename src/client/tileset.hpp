#pragma once

#include <shared/tile_defs.hpp>

#include <raylib.h>
#include <string>

struct tile_type {
    enum {
        passable = og::shared::tile_passable,
        wall = og::shared::tile_wall,
        water = og::shared::tile_water,
        chair = og::shared::tile_chair,
        near_water = og::shared::tile_near_water,
        swamp = og::shared::tile_swamp,
        jump = og::shared::tile_jump,
        bed_top = og::shared::tile_bed_top,
        bed_bottom = og::shared::tile_bed_bottom,

        unknown = 1 << 16
    };
};

class tileset {
  public:
    explicit tileset(const std::string &filename, int format = og::shared::tileset_format_pics1);

    [[nodiscard]] auto get_texture() const -> Texture2D { return texture_; }
    [[nodiscard]] auto get_tile_width() const -> float { return tile_width_; }
    [[nodiscard]] auto get_tile_height() const -> float { return tile_height_; }
    [[nodiscard]] auto get_format() const -> int { return format_; }
    [[nodiscard]] auto get_tile_type(int tile_id) const -> int;

  private:
    int format_ = og::shared::tileset_format_pics1;
    int tile_count_ = 0;
    og::shared::tile_defs defs_;
    Texture2D texture_;
    float tile_width_ = 0.0f;
    float tile_height_ = 0.0f;
};
