#include "tileset.hpp"

#include "texture_manager.hpp"

#include <cstdint>
#include <fstream>

namespace {
    constexpr auto to_le(const uint32_t i) -> uint32_t {
        return (i >> 24 & 0xFF) | (i >> 8 & 0xFF00);
    }

    auto skip_array(std::ifstream &stream) -> void {
        uint32_t size;

        stream.read(reinterpret_cast<char *>(&size), 4);
        if (!stream) {
            return;
        }

        const auto array_length = to_le(size);

        stream.seekg(array_length * 4, std::ios_base::cur);
    }

    auto load_array(std::ifstream &stream, int *tiles, const int tile_count, const int type) -> void {
        uint32_t size;

        stream.read(reinterpret_cast<char *>(&size), 4);
        if (!stream) {
            return;
        }

        const auto array_length = to_le(size);
        const auto array_data = new uint32_t[array_length];

        stream.read(reinterpret_cast<char *>(array_data), array_length * 4);

        for (auto i = 0; i < array_length; ++i) {
            const auto tile_data = array_data[i];
            const auto tile = to_le(tile_data);

            if (tile >= tile_count) {
                continue;
            }

            if (type == tile_type::passable) {
                tiles[tile] = type;
            } else {
                tiles[tile] = type;
            }
        }

        delete[] array_data;
    }

    auto load_arrays(const int tile_count) -> int * {
        std::ifstream stream("arrays.dat", std::ios::binary);

        const auto tiles = new int[tile_count];
        for (auto i = 0; i < tile_count; ++i) {
            tiles[i] = tile_type::wall;
        }

        load_array(stream, tiles, tile_count, tile_type::passable);
        load_array(stream, tiles, tile_count, tile_type::water);
        skip_array(stream);
        skip_array(stream);
        load_array(stream, tiles, tile_count, tile_type::chair);
        load_array(stream, tiles, tile_count, tile_type::bed_top);
        load_array(stream, tiles, tile_count, tile_type::bed_bottom);
        load_array(stream, tiles, tile_count, tile_type::swamp);
        // load_array(stream, tiles, tile_count, tile_type::water_shallow);
        // load_array(stream, tiles, tile_count, tile_type::jump);
        // load_array(stream, tiles, tile_count, tile_type::swamp);

        stream.close();

        return tiles;
    }
}

tileset::tileset(const std::string &filename) : texture_(load_texture(filename)) {
    tile_width_ = 16.0f / static_cast<float>(texture_.width);
    tile_height_ = 16.0f / static_cast<float>(texture_.height);

    const auto tiles_x = texture_.width / 16;
    const auto tiles_y = texture_.height / 16;

    tile_count_ = tiles_x * tiles_y;
    tiles_ = load_arrays(tile_count_);
}

tileset::~tileset() {
    if (tiles_ == nullptr) {
        return;
    }

    delete[] tiles_;

    tiles_ = nullptr;
}

auto tileset::get_tile_type(const int tile_id) const -> int {
    if (tiles_ == nullptr || tile_id < 0 || tile_id >= tile_count_) {
        return tile_type::passable;
    }

    return tiles_[tile_id];
}
