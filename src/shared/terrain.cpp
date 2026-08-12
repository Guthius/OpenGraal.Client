#include <shared/terrain.hpp>

#include <array>
#include <cmath>
#include <limits>

using namespace std;

namespace og::shared {
    namespace {
        constexpr int level_size = 64;
        constexpr int octaves = 5;

        constexpr std::array<terrain_band, 6> default_bands = {
            {
             terrain_band{.ceiling = -6.0, .block = 34, .type = tile_water},
             {.ceiling = 2.0, .block = 42, .type = tile_near_water},
             {.ceiling = 30.0, .block = 1058, .type = tile_passable},
             {.ceiling = 55.0, .block = 621, .type = tile_passable},
             {.ceiling = 80.0, .block = 1570, .type = tile_passable},
             {.ceiling = 1e9, .block = 1561, .type = tile_passable},
             }
        };

        auto noise_at(const int x, const int y, const uint32_t seed) -> double {
            auto hash = (static_cast<uint32_t>(x) * 374761393u) + (static_cast<uint32_t>(y) * 668265263u) + seed;
            hash = (hash ^ (hash >> 13)) * 1274126177u;
            hash ^= hash >> 16;

            return (static_cast<double>(hash) / static_cast<double>(numeric_limits<uint32_t>::max())) - 0.5;
        }

        auto smooth(const double t) -> double {
            return t * t * (3.0 - (2.0 * t));
        }

        auto lattice_noise(const double x, const double y, const int period, const uint32_t seed) -> double {
            const auto gx = static_cast<int>(floor(x / period));
            const auto gy = static_cast<int>(floor(y / period));
            const auto fx = smooth((x / period) - gx);
            const auto fy = smooth((y / period) - gy);

            const auto top = lerp(noise_at(gx, gy, seed), noise_at(gx + 1, gy, seed), fx);
            const auto bottom = lerp(noise_at(gx, gy + 1, seed), noise_at(gx + 1, gy + 1, seed), fx);

            return lerp(top, bottom, fy);
        }
    }

    auto default_terrain_layout() -> const terrain_layout & {
        static const terrain_layout layout{
            .bands = {begin(default_bands), end(default_bands)}
        };

        return layout;
    }

    auto generate_height_field(const gmap_data &map, const gmap_position cell) -> vector<double> {
        const auto stride = map.width + 1;
        if (map.height_map.size() < static_cast<size_t>(stride) * (map.height + 1)) {
            return {};
        }

        const auto corner = [&](const int x, const int y) {
            return static_cast<double>(map.height_map[(static_cast<size_t>(y) * stride) + x]);
        };

        const auto top_left = corner(cell.x, cell.y);
        const auto top_right = corner(cell.x + 1, cell.y);
        const auto bottom_left = corner(cell.x, cell.y + 1);
        const auto bottom_right = corner(cell.x + 1, cell.y + 1);

        const auto amplitude = map.level_height > 0.0 ? map.level_height : 1.0;
        const auto persistence = map.level_chaos > 0.0 ? map.level_chaos : 0.5;
        const auto seed = map.random_seeds.empty() ? 0u : map.random_seeds.front();

        vector<double> field(static_cast<size_t>(level_size) * level_size);

        for (auto y = 0; y < level_size; ++y) {
            const auto fy = (y + 0.5) / level_size;

            for (auto x = 0; x < level_size; ++x) {
                const auto fx = (x + 0.5) / level_size;

                const auto base = lerp(
                    lerp(top_left, top_right, fx),
                    lerp(bottom_left, bottom_right, fx),
                    fy);

                const auto world_x = static_cast<double>(cell.x * level_size) + x;
                const auto world_y = static_cast<double>(cell.y * level_size) + y;

                auto detail = 0.0;
                auto scale = amplitude;
                auto period = level_size;

                for (auto octave = 0; octave < octaves && period > 1; ++octave) {
                    detail += lattice_noise(world_x, world_y, period, seed) * scale;
                    scale *= persistence;
                    period /= 2;
                }

                field[(static_cast<size_t>(y) * level_size) + x] = base + detail;
            }
        }

        return field;
    }

    auto generate_level_tiles(const gmap_data &map, const gmap_position cell, const terrain_layout &layout) -> vector<uint16_t> {
        const auto field = generate_height_field(map, cell);
        if (field.empty() || layout.bands.empty()) {
            return {};
        }

        vector<uint16_t> tiles(field.size());

        for (size_t i = 0; i < field.size(); ++i) {
            auto chosen = layout.bands.back().block;

            for (const auto &band : layout.bands) {
                if (field[i] <= band.ceiling) {
                    chosen = band.block;

                    break;
                }
            }

            tiles[i] = static_cast<uint16_t>(chosen);
        }

        return tiles;
    }
}

namespace og::shared {
    auto terrain_tile_defs(const int tile_count, const terrain_layout &layout) -> tile_defs {
        if (tile_count <= 0) {
            return {};
        }

        vector types(static_cast<size_t>(tile_count), static_cast<int>(tile_passable));

        for (const auto &band : layout.bands) {
            if (band.block >= 0 && static_cast<size_t>(band.block) < types.size()) {
                types[band.block] = band.type;
            }
        }

        return tile_defs(std::move(types));
    }
}
