#include "editor_palette.hpp"

#include "../constants.hpp"
#include "../dev_input.hpp"
#include "editor_profiles.hpp"

#include <algorithm>
#include <cmath>
#include <raylib.h>

namespace {
    constexpr double double_click_seconds = 0.35;

    auto tile_for_cell(const int col, const int row) -> og::shared::tile_id {
        return static_cast<og::shared::tile_id>(((col / 16) * 512) + (row * 16) + (col % 16));
    }
}

auto editor_palette::cell_at(const Vector2 pt) const -> std::pair<int, int> {
    const auto *tiles = tileset_source ? tileset_source() : nullptr;
    if (tiles == nullptr) {
        return {0, 0};
    }

    const auto texture = tiles->get_texture();
    const auto image = view_.screen_to_level(pt);
    const auto col = static_cast<int>(std::floor(image.x / tile_size));
    const auto row = static_cast<int>(std::floor(image.y / tile_size));

    return {
        std::clamp(col, 0, std::max(0, (texture.width / tile_size) - 1)),
        std::clamp(row, 0, std::max(0, (texture.height / tile_size) - 1)),
    };
}

void editor_palette::update(const float dt) {
    gui_control::update(dt);

    if (is_mouse_over() && !panning_ && !selecting_) {
        if (const auto wheel = dev_input::mouse_wheel(); wheel != 0.0f) {
            const auto mouse = dev_input::mouse_position();

            view_.zoom_step(wheel > 0.0f ? 1 : -1, global_to_local_coord(mouse.x, mouse.y));
        }
    }
}

void editor_palette::on_mouse_pressed(const int button, const Vector2 pt) {
    if (button == MOUSE_BUTTON_MIDDLE) {
        panning_ = true;
        pan_start_ = pt;
        pan_origin_ = {view_.pan_x, view_.pan_y};

        capture_mouse();

        return;
    }

    if (button != MOUSE_BUTTON_LEFT) {
        return;
    }

    const auto [col, row] = cell_at(pt);

    if (GetTime() - last_click_at_ < double_click_seconds && col == last_click_col_ && row == last_click_row_) {
        background_tile_ = tile_for_cell(col, row);

        if (background_picked) {
            background_picked(background_tile_);
        }
    }

    last_click_at_ = GetTime();
    last_click_col_ = col;
    last_click_row_ = row;

    selecting_ = true;
    anchor_col_ = cursor_col_ = col;
    anchor_row_ = cursor_row_ = row;

    capture_mouse();
}

void editor_palette::on_mouse_move(const Vector2 pt) {
    if (panning_) {
        view_.pan_x = pan_origin_.x + ((pan_start_.x - pt.x) / view_.zoom);
        view_.pan_y = pan_origin_.y + ((pan_start_.y - pt.y) / view_.zoom);

        return;
    }

    if (!selecting_) {
        return;
    }

    const auto [col, row] = cell_at(pt);

    cursor_col_ = col;
    cursor_row_ = row;
}

void editor_palette::on_mouse_released(const int button, const Vector2 /*pt*/) {
    if (panning_ && button == MOUSE_BUTTON_MIDDLE) {
        panning_ = false;
        release_mouse();

        return;
    }

    if (!selecting_ || button != MOUSE_BUTTON_LEFT) {
        return;
    }

    selecting_ = false;

    release_mouse();

    const auto left = std::min(anchor_col_, cursor_col_);
    const auto top = std::min(anchor_row_, cursor_row_);
    const auto width = std::abs(cursor_col_ - anchor_col_) + 1;
    const auto height = std::abs(cursor_row_ - anchor_row_) + 1;

    auto brush = tile_clipboard{.width = width, .height = height};
    brush.tiles.reserve(static_cast<size_t>(width) * static_cast<size_t>(height));

    for (int row = top; row < top + height; ++row) {
        for (int col = left; col < left + width; ++col) {
            brush.tiles.push_back(tile_for_cell(col, row));
        }
    }

    if (brush_selected) {
        brush_selected(brush);
    }
}

void editor_palette::draw() const {
    const auto bounds = get_bounds();

    if (const auto profile = get_profile()) {
        profile->draw_frame(bounds, false, false);
    }

    const auto *tiles = tileset_source ? tileset_source() : nullptr;
    if (tiles == nullptr) {
        draw_children();

        return;
    }

    const auto texture = tiles->get_texture();

    const auto clip = local_to_global_coord({0, 0});

    BeginScissorMode(
        static_cast<int>(clip.x) + 1, static_cast<int>(clip.y) + 1,
        std::max(0, static_cast<int>(bounds.width) - 2), std::max(0, static_cast<int>(bounds.height) - 2));

    const auto origin = view_.level_to_screen({0, 0});

    DrawTexturePro(texture,
        {0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)},
        {bounds.x + origin.x, bounds.y + origin.y,
            static_cast<float>(texture.width) * view_.zoom, static_cast<float>(texture.height) * view_.zoom},
        {0, 0}, 0.0f, WHITE);

    if (last_click_col_ >= 0) {
        const auto left = std::min(anchor_col_, cursor_col_);
        const auto top = std::min(anchor_row_, cursor_row_);
        const auto width = std::abs(cursor_col_ - anchor_col_) + 1;
        const auto height = std::abs(cursor_row_ - anchor_row_) + 1;
        const auto corner = view_.level_to_screen({static_cast<float>(left * tile_size), static_cast<float>(top * tile_size)});

        editor_profiles::begin_invert();
        DrawRectangleLinesEx(
            {
                bounds.x + corner.x,
                bounds.y + corner.y,
                static_cast<float>(width * tile_size) * view_.zoom,
                static_cast<float>(height * tile_size) * view_.zoom,
            },
            2, WHITE);
        editor_profiles::end_invert();
    }

    EndScissorMode();

    draw_children();
}
