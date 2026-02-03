#pragma once

#include <raylib.h>
#include <string>

struct tile_type {
    enum {
        passable = 0,
        wall = 1 << 0,
        water = 1 << 1,
        chair = 1 << 2,
        near_water = 1 << 3,
        swamp = 1 << 4,
        jump = 1 << 5,
        bed_top = 1 << 6,
        bed_bottom = 1 << 7,

        unknown = 1 << 16
    };
};

class tileset {
  public:
    explicit tileset(const std::string &filename);

    ~tileset();

    [[nodiscard]] auto get_texture() const -> Texture2D { return texture_; }
    [[nodiscard]] auto get_tile_width() const -> float { return tile_width_; }
    [[nodiscard]] auto get_tile_height() const -> float { return tile_height_; }
    [[nodiscard]] auto get_tile_type(int tile_id) const -> int;

  private:
    int tile_count_;
    int *tiles_;
    Texture2D texture_;
    float tile_width_;
    float tile_height_;
};
