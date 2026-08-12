#include <shared/tile_defs.hpp>

#include <shared/terrain.hpp>

#include <array>
#include <bit>
#include <cstdint>
#include <format>
#include <fstream>

using namespace std;

namespace og::shared {
    namespace {
        auto read_count(istream &is) -> uint32_t {
            uint32_t raw = 0;

            is.read(reinterpret_cast<char *>(&raw), sizeof(raw));
            if (!is) {
                return 0;
            }

            return byteswap(raw);
        }

        void skip_array(istream &is) {
            const auto count = read_count(is);

            is.seekg(static_cast<streamoff>(count) * sizeof(uint32_t), ios_base::cur);
        }

        constexpr size_t tileset_columns = 16;
        constexpr size_t tiles_per_block = 512;

        auto to_tile_type(const uint8_t graal_type) -> int {
            switch (graal_type) {
            case 3:  return tile_chair;
            case 4:  return tile_bed_top;
            case 5:  return tile_bed_bottom;
            case 6:
            case 7:  return tile_swamp;
            case 8:
            case 9:  return tile_near_water;
            case 11:
            case 12: return tile_water;
            case 20:
            case 22: return tile_wall;
            case 21: return tile_jump;
            default: return tile_passable;
            }
        }

        void read_array(istream &is, vector<int> &types, const int type) {
            const auto count = read_count(is);
            if (count == 0) {
                return;
            }

            vector<uint32_t> entries(count);
            is.read(reinterpret_cast<char *>(entries.data()), static_cast<streamsize>(count * sizeof(uint32_t)));

            for (const auto raw : entries) {
                const auto id = byteswap(raw);
                if (id < types.size()) {
                    types[id] = type;
                }
            }
        }
    }

    tile_defs::tile_defs(vector<int> types) : types_(std::move(types)) {
    }

    auto tile_defs::type_of(const int id) const -> int {
        if (id < 0 || static_cast<size_t>(id) >= types_.size()) {
            return tile_passable;
        }

        return types_[static_cast<size_t>(id)];
    }

    auto load_tile_defs(istream &is, const int tile_count) -> result<tile_defs> {
        if (tile_count <= 0) {
            return make_error("tile count must be positive");
        }

        vector types(static_cast<size_t>(tile_count), static_cast<int>(tile_wall));

        read_array(is, types, tile_passable);
        read_array(is, types, tile_water);
        read_array(is, types, tile_wall);
        skip_array(is);
        read_array(is, types, tile_chair);
        read_array(is, types, tile_bed_top);
        read_array(is, types, tile_bed_bottom);
        read_array(is, types, tile_near_water);

        return tile_defs(std::move(types));
    }

    auto tile_defs_for_format(const int format, const int tile_count) -> tile_defs {
        if (format == tileset_format_terrain) {
            return terrain_tile_defs(tile_count);
        }

        if (format != tileset_format_new_world || tile_count <= 0) {
            return {};
        }

        static constexpr array<array<uint8_t, 32>, 8> layout = {
            {
             {1, 1, 1, 1, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 1, 1, 3, 3, 3, 3, 4, 5, 6, 6, 1, 1, 21, 21},
             {22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 6, 6, 7, 7, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 1, 1},
             {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22},
             {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22},
             {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22},
             {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22},
             {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22},
             {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22},
             }
        };

        vector types(static_cast<size_t>(tile_count), static_cast<int>(tile_passable));

        for (size_t id = 0; id < types.size(); ++id) {
            const auto block = id / tiles_per_block;
            if (block >= layout.size()) {
                continue;
            }

            types[id] = to_tile_type(layout[block][id % tiles_per_block / tileset_columns]);
        }

        return tile_defs(std::move(types));
    }

    auto load_tile_defs(const filesystem::path &path, const int tile_count) -> result<tile_defs> {
        ifstream ifs(path, ios::binary);
        if (!ifs) {
            return make_error(format("could not open '{}'", path.string()));
        }

        return load_tile_defs(ifs, tile_count);
    }
}
