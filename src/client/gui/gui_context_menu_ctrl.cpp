#include "gui_context_menu_ctrl.hpp"

void gui_context_menu_ctrl::add_row(const int id, std::string text) {
    rows_.push_back({.id = id, .text = std::move(text)});
}

void gui_context_menu_ctrl::clear_rows() {
    rows_.clear();
    selected_ = -1;
}

auto gui_context_menu_ctrl::row_height() const -> float {
    const auto profile = get_profile();

    return profile ? profile->get_font_size() + 6.0f : 18.0f;
}

void gui_context_menu_ctrl::open(const Vector2 position) {
    set_position(position);
    set_size({get_size().x, row_height() * static_cast<float>(rows_.size())});
    set_visible(true);
    bring_to_front();
}

void gui_context_menu_ctrl::on_mouse_released(const int button, const Vector2 pt) {
    if (!contains(pt) || rows_.empty()) {
        return;
    }

    const auto index = static_cast<size_t>((pt.y - get_bounds().y) / row_height());
    if (index < rows_.size() && rows_[index].text != "-") {
        selected_ = rows_[index].id;
        close();

        if (clicked) {
            clicked();
        }
    }
}

void gui_context_menu_ctrl::draw() const {
    const auto profile = get_profile();
    if (!profile) {
        return;
    }

    const auto bounds = get_bounds();
    const auto height = row_height();

    profile->draw_frame(bounds, false, false);

    auto y = bounds.y;
    for (const auto &[id, text] : rows_) {
        if (text == "-") {
            DrawLineEx({bounds.x + 2, y + (height / 2)}, {bounds.x + bounds.width - 2, y + (height / 2)}, 1.0f, profile->border_color);
        } else {
            profile->draw_text(text, {bounds.x + 4, y, bounds.width - 8, height}, alignment::left);
        }

        y += height;
    }

    draw_children();
}
