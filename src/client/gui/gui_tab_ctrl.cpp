#include "gui_tab_ctrl.hpp"

#include "../font_manager.hpp"

namespace {
    constexpr int sprite_unselected = 0;
    constexpr int sprite_selected = 6;

    constexpr float text_padding = 10.0f;
    constexpr float top_padding = 2.0f;
    constexpr float fallback_cap_height = 16.0f;
}

auto gui_tab_ctrl::get_selected_row() const -> int {
    return selected_ >= 0 && selected_ < static_cast<int>(rows_.size()) ? rows_[selected_].id : -1;
}

void gui_tab_ctrl::clear_rows() {
    rows_.clear();
    selected_ = -1;
}

void gui_tab_ctrl::add_row(const int id, const std::string &text) {
    rows_.push_back({.id = id, .text = text});
}

void gui_tab_ctrl::set_selected_row(const int id) {
    for (size_t i = 0; i < rows_.size(); ++i) {
        if (rows_[i].id != id) {
            continue;
        }

        if (static_cast<int>(i) == selected_) {
            return;
        }

        if (selected_ >= 0 && selected_ < static_cast<int>(rows_.size()) && deselected) {
            deselected(rows_[selected_].id, rows_[selected_].text, selected_);
        }

        selected_ = static_cast<int>(i);

        if (selected) {
            selected(rows_[i].id, rows_[i].text, selected_);
        }

        return;
    }
}

auto gui_tab_ctrl::width_of(const row &row) const -> float {
    const auto profile = get_profile();
    if (!profile) {
        return 0.0f;
    }

    const auto size = profile->get_font_size();
    const auto font = load_font(profile->get_font(), size);

    return MeasureTextEx(font, row.text.c_str(), size, 1).x + (text_padding * 2.0f);
}

auto gui_tab_ctrl::row_at(const Vector2 pt) const -> int {
    if (!contains(pt)) {
        return -1;
    }

    auto x = 0.0f;

    for (size_t i = 0; i < rows_.size(); ++i) {
        const auto width = width_of(rows_[i]);

        if (pt.x >= x && pt.x < x + width) {
            return static_cast<int>(i);
        }

        x += width;
    }

    return -1;
}

void gui_tab_ctrl::on_mouse_pressed(int, const Vector2 pt) {
    if (const auto index = row_at(pt); index >= 0) {
        set_selected_row(rows_[index].id);
    }
}

void gui_tab_ctrl::draw() const {
    const auto profile = get_profile();
    if (!profile) {
        return;
    }

    const auto bounds = get_bounds();
    const auto texture = profile->get_texture();
    const auto size = profile->get_font_size();
    const auto font = load_font(profile->get_font(), size);

    auto x = bounds.x;

    for (size_t i = 0; i < rows_.size(); ++i) {
        const auto width = width_of(rows_[i]);
        const auto base = static_cast<int>(i) == selected_ ? sprite_selected : sprite_unselected;
        const auto left = profile->get_sprite_rect(base);
        const auto middle = profile->get_sprite_rect(base + 1);
        const auto right = profile->get_sprite_rect(base + 2);
        
        const auto top = bounds.y + top_padding;
        const auto cap_height = bounds.height > top_padding ? bounds.height - top_padding : fallback_cap_height;
        const auto middle_width = std::max(0.0f, width - left.width - right.width);

        if (IsTextureValid(texture) && left.width > 0) {
            DrawTexturePro(texture, left, {x, top, left.width, cap_height}, {0, 0}, 0.0f, WHITE);
            DrawTexturePro(texture, middle, {x + left.width, top, middle_width, cap_height}, {0, 0}, 0.0f, WHITE);
            DrawTexturePro(texture, right, {x + left.width + middle_width, top, right.width, cap_height}, {0, 0}, 0.0f, WHITE);
        } else {
            DrawRectangleRec({x, top, width, cap_height},
                static_cast<int>(i) == selected_ ? profile->fill_color_hl : profile->fill_color);
        }

        const auto text_width = MeasureTextEx(font, rows_[i].text.c_str(), size, 1).x;

        DrawTextEx(font, rows_[i].text.c_str(),
            {x + ((width - text_width) / 2.0f), top + ((cap_height - size) / 2.0f)},
            size, 1, profile->font_color);

        x += width;
    }

    draw_children();
}
