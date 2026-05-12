#include "level.hpp"

#include "utils.hpp"

#include <boost/algorithm/string.hpp>
#include <cmath>
#include <fstream>
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
            const auto tilex = ((tileId / 512) * 16 + (tileId % 16)) * 16;
            const auto tiley = ((tileId % 512) / 16) * 16;

            const auto tx1 = static_cast<float>(tilex) / 2048.0f; // Left
            const auto tx2 = tx1 + tile_width;                    // Right
            const auto ty1 = static_cast<float>(tiley) / 512.0f;  // Top
            const auto ty2 = ty1 + tile_height;                   // Bottom

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
        auto &rect = link.get_rectangle();

        if (x >= rect.x && x <= rect.x + rect.width &&
            y >= rect.y && y <= rect.y + rect.height) {
            return &link;
        }
    }

    return nullptr;
}

auto level::get_sign_at(const float x, const float y) const -> const level_sign * {
    for (const auto &sign : signs_) {
        auto &rect = sign.get_rectangle();

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

    const auto tileIndex = ty * 64 + tx;
    const auto tileId = board_[tileIndex];

    return tileset->get_tile_type(tileId);
}

auto level::get_tile_id(const int x, const int y) const -> int {
    const auto tx = x / 16;
    const auto ty = y / 16;

    if (tx < 0 || tx > 63 || ty < 0 || ty > 63) {
        return -1;
    }

    const auto tileIndex = ty * 64 + tx;
    return board_[tileIndex];
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
            const int tile_index = y * 64 + x;

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

    const auto tile_index = y * 64 + x;
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

                if (tile_ids[yy * 2 + xx] != board_[ty * 64 + tx]) {
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

                board_[ty * 64 + tx] = tile_ids[yy * 2 + xx];
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

                board_[ty * 64 + tx] = tile_ids[yy * 2 + xx];
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

auto level::load(const std::filesystem::path &path) -> std::shared_ptr<level> {
    if (!is_regular_file(path)) {
        return nullptr;
    }

    auto stream = std::ifstream(path, std::ios::binary);
    if (!stream) {
        return nullptr;
    }

    char version[9]{};

    stream.read(version, 8);
    if (!stream) {
        return nullptr;
    }

    if (TextIsEqual(version, "GLEVNW01")) {
        return load_nw(stream);
    }

    return load_graal(stream, version);
}

auto level::load_nw(std::ifstream &stream) -> std::shared_ptr<level> {
    static std::string base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

    std::vector<short> board(64 * 64);

    std::string line;
    while (std::getline(stream, line)) {
        auto tokens = split_string(line);

        if (tokens.empty() || tokens[0] != "BOARD") {
            continue;
        }

        if (tokens.size() != 6) {
            continue;
        }

        const auto x = std::stoi(tokens[1]);
        const auto y = std::stoi(tokens[2]);
        const auto w = std::stoi(tokens[3]);
        const auto z = std::stoi(tokens[4]);

        if (x < 0 || x >= 64 || y < 0 || y >= 64) {
            continue;
        }

        if (z != 0) {
            continue;
        }

        auto &data = tokens[5];
        if (data.size() < w * 2) {
            continue;
        }

        for (int i = 0; i < w; ++i) {
            const auto j = i * 2;

            const auto b1 = base64.find_first_of(data[j]) << 6;
            const auto b2 = base64.find_first_of(data[j + 1]);

            const auto tileId = b1 | b2;
            const auto tileIndex = (y * 64) + (x + i);

            board[tileIndex] = static_cast<short>(tileId);
        }
    }

    constexpr std::vector<level_link> links;
    constexpr std::vector<level_sign> signs;

    return std::make_shared<level>(board, links, signs);
}

auto level::load_graal(std::ifstream &stream, const int bits, const size_t code_mask, const size_t control_bit, const bool has_chests) -> std::shared_ptr<level> {
    constexpr int boardSize = 64 * 64;

    int bitsRead = 0;
    char byte;
    size_t buf = 0;
    short tile1 = -1;
    int boardIndex = 0;
    bool doubleMode = false;
    int count = 1;
    std::vector<short> board(64 * 64);

    while (boardIndex < boardSize && !stream.eof()) {
        while (bitsRead < bits) {
            stream.read(&byte, 1);

            buf |= static_cast<uint8_t>(byte) << bitsRead;

            bitsRead += 8;
        }

        const uint16_t code = buf & code_mask;
        buf >>= bits;
        bitsRead -= bits;

        if (code & control_bit) {
            doubleMode = (code & 0x100) == 0x100;
            count = code & 0xFF;
            continue;
        }

        if (doubleMode) {
            if (tile1 == -1) {
                tile1 = static_cast<short>(code);
                continue;
            }

            const auto tile2 = static_cast<short>(code);

            for (auto i = 0; i < count && boardIndex < boardSize - 1; ++i) {
                board[boardIndex++] = tile1;
                board[boardIndex++] = tile2;
            }

            tile1 = -1;
            doubleMode = false;
        } else {
            for (auto i = 0; i < count && boardIndex < boardSize; ++i) {
                board[boardIndex++] = static_cast<short>(code);
            }
        }

        count = 1;
    }

    std::vector<level_link> links;
    std::vector<level_sign> signs;
    std::string line;

    while (std::getline(stream, line)) {
        boost::trim(line);
        if (line.empty() || line == "#") {
            break;
        }

        links.emplace_back(line);
    }

    char baddyX;
    char baddyY;
    char baddyType;

    /* Read Baddies */
    while (!stream.eof()) {
        stream.read(&baddyX, 1);
        stream.read(&baddyY, 1);
        stream.read(&baddyType, 1);

        std::getline(stream, line);

        if (!stream) {
            break;
        }

        if (baddyX == -1 && baddyY == -1 && baddyType == -1) {
            break;
        }
    }

    /* Read NPC's */
    while (std::getline(stream, line)) {
        boost::trim(line);
        if (line.empty() || line == "#") {
            break;
        }
    }

    /* Read Chests */
    if (has_chests) {
        while (std::getline(stream, line)) {
            boost::trim(line);
            if (line.empty() || line == "#") {
                break;
            }
        }
    }

    /* Read Signs */
    while (std::getline(stream, line)) {
        if (line.empty()) {
            break;
        }

        const auto x = static_cast<float>(line[0] - 32);
        const auto y = static_cast<float>(line[1] - 32);
        auto text = decode_sign_text(line.substr(2));

        signs.emplace_back(x * 16, y * 16, text);
    }

    return std::make_shared<level>(board, links, signs);
}

auto level::load_graal(std::ifstream &stream, const char *version) -> std::shared_ptr<level> {
    auto v = -1;

    if (TextIsEqual(version, "GR-V1.00"))
        v = 0;
    else if (TextIsEqual(version, "GR-V1.01"))
        v = 1;
    else if (TextIsEqual(version, "GR-V1.02"))
        v = 2;
    else if (TextIsEqual(version, "GR-V1.03"))
        v = 3;

    if (v == -1) {
        return nullptr;
    }

    return load_graal(
        stream,
        v > 0 ? 13 : 12,
        v > 0 ? 0x1FFF : 0xFFF,
        v > 0 ? 0x1000 : 0x800,
        v > 0);
}
