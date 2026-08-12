#include "editor_ops.hpp"

#include <algorithm>
#include <utility>
#include <vector>

using og::shared::level_data;
using og::shared::level_height;
using og::shared::level_layer;
using og::shared::level_tile_count;
using og::shared::level_width;
using og::shared::tile_id;

namespace editor_ops {
    auto board(level_data &level) -> std::vector<tile_id> & {
        for (auto &layer : level.layers) {
            if (layer.index == 0) {
                return layer.tiles;
            }
        }

        level.layers.push_back(level_layer{
            .index = 0,
            .tiles = std::vector<tile_id>(level_tile_count, 0),
        });

        return level.layers.back().tiles;
    }

    auto copy_rect(level_data &level, const int x, const int y, const int width, const int height) -> tile_clipboard {
        auto &tiles = board(level);
        auto copied = tile_clipboard{.width = width, .height = height};
        copied.tiles.reserve(static_cast<size_t>(width) * static_cast<size_t>(height));

        for (int yy = y; yy < y + height; ++yy) {
            for (int xx = x; xx < x + width; ++xx) {
                const auto inside = xx >= 0 && xx < level_width && yy >= 0 && yy < level_height;

                copied.tiles.push_back(inside ? tiles[(yy * level_width) + xx] : 0);
            }
        }

        return copied;
    }

    void paste(level_data &level, const int x, const int y, const tile_clipboard &source) {
        auto &tiles = board(level);

        for (int row = 0; row < source.height; ++row) {
            for (int column = 0; column < source.width; ++column) {
                const auto xx = x + column;
                const auto yy = y + row;

                if (xx >= 0 && xx < level_width && yy >= 0 && yy < level_height) {
                    tiles[(yy * level_width) + xx] = source.tiles[(row * source.width) + column];
                }
            }
        }
    }

    void fill_rect(level_data &level, const int x, const int y, const int width, const int height, const tile_id tile) {
        auto &tiles = board(level);

        for (int yy = std::max(0, y); yy < std::min(level_height, y + height); ++yy) {
            for (int xx = std::max(0, x); xx < std::min(level_width, x + width); ++xx) {
                tiles[(yy * level_width) + xx] = tile;
            }
        }
    }

    void flood_fill(level_data &level, const int x, const int y, const tile_id tile) {
        if (x < 0 || x >= level_width || y < 0 || y >= level_height) {
            return;
        }

        auto &tiles = board(level);
        const auto target = tiles[(y * level_width) + x];

        if (target == tile) {
            return;
        }

        std::vector<std::pair<int, int>> pending{
            {x, y}
        };

        while (!pending.empty()) {
            const auto [cx, cy] = pending.back();
            pending.pop_back();

            if (cx < 0 || cx >= level_width || cy < 0 || cy >= level_height || tiles[(cy * level_width) + cx] != target) {
                continue;
            }

            tiles[(cy * level_width) + cx] = tile;

            pending.emplace_back(cx + 1, cy);
            pending.emplace_back(cx - 1, cy);
            pending.emplace_back(cx, cy + 1);
            pending.emplace_back(cx, cy - 1);
        }
    }

    auto snapshot(level_data &level) -> tile_patch {
        return tile_patch{
            .x = 0,
            .y = 0,
            .width = level_width,
            .height = level_height,
            .before = board(level),
        };
    }
}
