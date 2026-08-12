#include "level.hpp"

#include "constants.hpp"

#include <shared/level.hpp>

#include <sstream>

#include <cmath>
#include <rlgl.h>

namespace {
    struct tile_pattern {
        tile_pattern_type type;
        std::array<short, 4> tiles;
        std::array<short, 4> replacement;
        leap_effect_type leap_type;
    };

    const std::vector<tile_pattern> patterns = {
        {.type = tile_pattern_type::bush,
         .tiles = {2, 3, 18, 19},
         .replacement = {677, 678, 693, 694},
         .leap_type = leap_effect_type::bush},
        {.type = tile_pattern_type::swamp,
         .tiles = {420, 421, 436, 437},
         .replacement = {679, 680, 695, 696},
         .leap_type = leap_effect_type::swamp},
        {.type = tile_pattern_type::vase,
         .tiles = {684, 685, 700, 701},
         .replacement = {1770, 1771, 1786, 1787}},
        {.type = tile_pattern_type::sign,
         .tiles = {512, 513, 528, 529},
         .replacement = {1802, 1803, 1818, 1819}},
    };
}

void level::set_board(const std::vector<uint16_t> &tiles) {
    if (tiles.size() != board_.size()) {
        return;
    }

    board_.assign(tiles.begin(), tiles.end());
    has_tiles_ = true;
}

level::level(const og::shared::level_data &data)
    : name_(data.name),
      board_(og::shared::level_tile_count, 0),
      npcs_(data.npcs),
      chests_(data.chests),
      baddies_(data.baddies) {
    if (const auto *tiles = data.tiles()) {
        board_.assign(tiles->begin(), tiles->end());
        has_tiles_ = true;
    }

    links_.reserve(data.links.size());
    for (const auto &link : data.links) {
        links_.emplace_back(link);
    }

    signs_.reserve(data.signs.size());
    for (const auto &sign : data.signs) {
        signs_.emplace_back(
            static_cast<float>(sign.x * tile_size),
            static_cast<float>(sign.y * tile_size),
            sign.text);
    }
}

void level::draw(const tileset *tileset) const {
    const auto tile_width = tileset->get_tile_width();
    const auto tile_height = tileset->get_tile_height();

    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255);

    for (int yy = 0; yy < 64; ++yy) {
        for (int xx = 0; xx < 64; ++xx) {
            const auto sx1 = static_cast<float>(xx * 16); // Left
            const auto sx2 = sx1 + 16;                    // Right
            const auto sy1 = static_cast<float>(yy * 16); // Top
            const auto sy2 = sy1 + 16;                    // Bottom

            const auto tileIndex = (yy * 64) + xx;
            const auto tileId = board_[tileIndex];
            const auto tilex = (((tileId / 512) * 16) + (tileId % 16)) * 16;
            const auto tiley = ((tileId % 512) / 16) * 16;

            const auto tx1 = static_cast<float>(tilex / 16) * tile_width;  // Left
            const auto tx2 = tx1 + tile_width;                             // Right
            const auto ty1 = static_cast<float>(tiley / 16) * tile_height; // Top
            const auto ty2 = ty1 + tile_height;                            // Bottom

            rlColor4ub(255, 255, 255, 255);

            rlTexCoord2f(tx1, ty1);
            rlVertex2f(sx1, sy1);

            rlTexCoord2f(tx1, ty2);
            rlVertex2f(sx1, sy2);

            rlTexCoord2f(tx2, ty2);
            rlVertex2f(sx2, sy2);

            rlTexCoord2f(tx2, ty1);
            rlVertex2f(sx2, sy1);
        }
    }

    rlEnd();
}

auto level::get_link_at(const float x, const float y) const -> const level_link * {
    for (const auto &link : links_) {
        const auto &rect = link.get_rectangle();

        if (x >= rect.x && x <= rect.x + rect.width &&
            y >= rect.y && y <= rect.y + rect.height) {
            return &link;
        }
    }

    return nullptr;
}

auto level::get_sign_at(const float x, const float y) const -> const level_sign * {
    for (const auto &sign : signs_) {
        const auto &rect = sign.get_rectangle();

        if (x >= rect.x && x <= rect.x + rect.width &&
            y >= rect.y && y <= rect.y + rect.height) {
            return &sign;
        }
    }

    return nullptr;
}

auto level::get_tile_type(const tileset *tileset, const int x, const int y) const -> int {
    const auto tx = x / 16;
    const auto ty = y / 16;

    if (tx < 0 || tx > 63 || ty < 0 || ty > 63) {
        return tile_type::passable;
    }

    const auto tile_index = (ty * 64) + tx;
    const auto tile_id = board_[tile_index];

    return tileset->get_tile_type(tile_id);
}

auto level::get_tile_id(const int x, const int y) const -> int {
    const auto tx = x / 16;
    const auto ty = y / 16;

    if (tx < 0 || tx > 63 || ty < 0 || ty > 63) {
        return -1;
    }

    const auto tile_index = (ty * 64) + tx;
    return board_[tile_index];
}

auto level::on_wall(const tileset *tileset, const Rectangle rect) const -> bool {
    if (constexpr float map_size = 64.0f * 16.0f;
        rect.x < 0 || rect.y < 0 ||
        rect.x + rect.width > map_size ||
        rect.y + rect.height > map_size) {
        return true;
    }

    auto sx = static_cast<int>(std::floor(rect.x / 16.0f));
    auto sy = static_cast<int>(std::floor(rect.y / 16.0f));
    auto dx = static_cast<int>(std::floor((rect.x + rect.width - 0.001f) / 16.0f));
    auto dy = static_cast<int>(std::floor((rect.y + rect.height - 0.001f) / 16.0f));

    sx = std::max(0, std::min(63, sx));
    sy = std::max(0, std::min(63, sy));
    dx = std::max(0, std::min(63, dx));
    dy = std::max(0, std::min(63, dy));

    for (int y = sy; y <= dy; ++y) {
        for (int x = sx; x <= dx; ++x) {
            const int tile_index = (y * 64) + x;

            if (const auto tile_id = board_[tile_index]; tileset->get_tile_type(tile_id) & tile_type::wall) {
                return true;
            }
        }
    }

    return false;
}

auto level::on_wall(const tileset *tileset, const Vector2 pt) const -> bool {
    if (constexpr float map_size = 64.0f * 16.0f;
        pt.x < 0.0f || pt.y < 0.0f ||
        pt.x >= map_size || pt.y >= map_size) {
        return true;
    }

    const auto x = static_cast<int>(pt.x / 16.0f);
    const auto y = static_cast<int>(pt.y / 16.0f);

    const auto tile_index = (y * 64) + x;
    const auto tile_id = board_[tile_index];

    return (tileset->get_tile_type(tile_id) & tile_type::wall) != 0;
}

auto level::find_tile_pattern_at(const float x, const float y) const -> tile_pattern_match {
    constexpr auto in_bounds = [&](const int dx, const int dy) -> bool {
        return dx >= 0 && dx < 64 && dy >= 0 && dy < 64;
    };

    const auto match_pattern_at = [&](const int dx, const int dy, const std::array<short, 4> &tile_ids) -> bool {
        for (auto yy = 0; yy < 2; yy++) {
            for (auto xx = 0; xx < 2; xx++) {
                const auto tx = dx + xx;
                const auto ty = dy + yy;

                if (!in_bounds(tx, ty)) {
                    return false;
                }

                if (tile_ids[(yy * 2) + xx] != board_[(ty * 64) + tx]) {
                    return false;
                }
            }
        }

        return true;
    };

    const auto tx = static_cast<int>(x / 16);
    const auto ty = static_cast<int>(y / 16);

    for (const auto &[object_type, tile_ids, replacement_tile_ids, leap_type] : patterns) {
        for (auto dy = ty - 1; dy <= ty; dy++) {
            for (auto dx = tx - 1; dx <= tx; dx++) {
                if (match_pattern_at(dx, dy, tile_ids)) {
                    return {
                        .type = object_type,
                        .x = dx,
                        .y = dy,
                        .replacement = {
                                        replacement_tile_ids[0],
                                        replacement_tile_ids[1],
                                        replacement_tile_ids[2],
                                        replacement_tile_ids[3]},
                        .leap_type = leap_type
                    };
                }
            }
        }
    }

    return {tile_pattern_type::none};
}

auto level::try_destroy_object_at(const float x, const float y) -> std::tuple<leap_effect_type, int, int> {
    const auto is_destructible = [&](const tile_pattern_type type) -> bool {
        return type == tile_pattern_type::swamp ||
               type == tile_pattern_type::bush;
    };

    const auto replace_tiles_at = [&](const int dx, const int dy, const std::array<short, 4> &tile_ids) {
        for (auto yy = 0; yy < 2; yy++) {
            for (auto xx = 0; xx < 2; xx++) {
                const auto tx = dx + xx;
                const auto ty = dy + yy;

                board_[(ty * 64) + tx] = tile_ids[(yy * 2) + xx];
            }
        }
    };

    const auto [object_type, dx, dy, replacement_tile_ids, leap_type] = find_tile_pattern_at(x, y);
    if (!is_destructible(object_type)) {
        return {leap_effect_type::none, 0, 0};
    }

    replace_tiles_at(dx, dy, replacement_tile_ids);

    return {leap_type, dx * 16, dy * 16};
}

auto level::try_lift_object_at(const float x, const float y) -> carry_object_type {
    const auto get_carried_item_type = [&](const tile_pattern_type type) -> carry_object_type {
        switch (type) {
        case tile_pattern_type::bush: return carry_object_type::bush;
        case tile_pattern_type::vase: return carry_object_type::vase;
        case tile_pattern_type::sign: return carry_object_type::sign;
        default:
            return carry_object_type::none;
        }
    };

    const auto replace_tiles_at = [&](const int dx, const int dy, const std::array<short, 4> &tile_ids) {
        for (auto yy = 0; yy < 2; yy++) {
            for (auto xx = 0; xx < 2; xx++) {
                const auto tx = dx + xx;
                const auto ty = dy + yy;

                board_[(ty * 64) + tx] = tile_ids[(yy * 2) + xx];
            }
        }
    };

    const auto [object_type, dx, dy, replacement_tile_ids, leap_type] = find_tile_pattern_at(x, y);

    const auto carried_item_type = get_carried_item_type(object_type);
    if (carried_item_type == carry_object_type::none) {
        return carry_object_type::none;
    }

    replace_tiles_at(dx, dy, replacement_tile_ids);

    return carried_item_type;
}

auto level::load(const std::string &name, const std::string &source) -> std::shared_ptr<level> {
    std::istringstream is(source);

    auto data = og::shared::load_level(name, is);
    if (!data) {
        TraceLog(LOG_WARNING, "LEVEL: %s", data.error().c_str());

        return nullptr;
    }

    return std::make_shared<level>(*data);
}

auto level::load(const std::filesystem::path &path) -> std::shared_ptr<level> {
    auto data = og::shared::load_level(path);
    if (!data) {
        TraceLog(LOG_WARNING, "LEVEL: %s", data.error().c_str());

        return nullptr;
    }

    return std::make_shared<level>(*data);
}
