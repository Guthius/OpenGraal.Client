#include "gui_popup_menu_ctrl.hpp"

#include "../font_manager.hpp"

#include <algorithm>

namespace {
    constexpr float row_padding = 2.0f;
    constexpr float text_padding = 4.0f;
    constexpr float arrow_width = 12.0f;
}

auto gui_popup_menu_ctrl::get_selected_id() const -> int {
    return selected_ >= 0 && selected_ < static_cast<int>(rows_.size()) ? rows_[selected_].id : -1;
}

auto gui_popup_menu_ctrl::get_selected_text() const -> std::string {
    return selected_ >= 0 && selected_ < static_cast<int>(rows_.size()) ? rows_[selected_].text : std::string{};
}

void gui_popup_menu_ctrl::clear_rows() {
    rows_.clear();
    selected_ = -1;
    open_ = false;

    gui_text_ctrl::set_text({});
}

void gui_popup_menu_ctrl::add_row(const int id, const std::string &text) {
    rows_.push_back({.id = id, .text = text});
}

void gui_popup_menu_ctrl::insert_row(const int index, const int id, const std::string &text) {
    const auto at = std::clamp(index, 0, static_cast<int>(rows_.size()));

    rows_.insert(rows_.begin() + at, {.id = id, .text = text});

    if (selected_ >= at) {
        ++selected_;
    }
}

void gui_popup_menu_ctrl::remove_row(const int id) {
    const auto match = std::ranges::find_if(rows_, [id](const row &entry) { return entry.id == id; });
    if (match == rows_.end()) {
        return;
    }

    if (const auto index = static_cast<int>(std::distance(rows_.begin(), match)); index <= selected_) {
        selected_ = index == selected_ ? -1 : selected_ - 1;
    }

    rows_.erase(match);

    gui_text_ctrl::set_text(get_selected_text());
}

void gui_popup_menu_ctrl::clear_selection() {
    selected_ = -1;

    gui_text_ctrl::set_text({});
}

void gui_popup_menu_ctrl::set_selected_index(const int index) {
    if (index >= 0 && index < static_cast<int>(rows_.size())) {
        select(index);
    }
}

void gui_popup_menu_ctrl::set_selected_id(const int id) {
    for (size_t i = 0; i < rows_.size(); ++i) {
        if (rows_[i].id == id) {
            select(static_cast<int>(i));

            return;
        }
    }
}

void gui_popup_menu_ctrl::select(const int index) {
    selected_ = index;

    gui_text_ctrl::set_text(rows_[index].text);

    if (selected) {
        selected(rows_[index].id, rows_[index].text, index);
    }

    if (clicked) {
        clicked();
    }
}

void gui_popup_menu_ctrl::open() {
    if (open_ || rows_.empty()) {
        return;
    }

    open_ = true;

    capture_mouse();
    bring_to_front();
}

void gui_popup_menu_ctrl::close() {
    if (!open_) {
        return;
    }

    open_ = false;

    release_mouse();
}

void gui_popup_menu_ctrl::on_blur() {
    close();
}

auto gui_popup_menu_ctrl::text_profile() const -> const std::shared_ptr<gui_control_profile> & {
    return text_profile_ ? text_profile_ : get_profile();
}

auto gui_popup_menu_ctrl::row_height() const -> float {
    const auto profile = text_profile();

    return (profile ? profile->get_font_size() : 12.0f) + row_padding;
}

auto gui_popup_menu_ctrl::popup_height() const -> float {
    return std::min(max_popup_height_, static_cast<float>(rows_.size()) * row_height());
}

auto gui_popup_menu_ctrl::row_at(const Vector2 pt) const -> int {
    const auto height = row_height();
    if (!open_ || height <= 0.0f || pt.x < 0 || pt.x >= get_bounds().width) {
        return -1;
    }

    const auto offset = pt.y - get_bounds().height;
    if (offset < 0 || offset >= popup_height()) {
        return -1;
    }

    const auto index = static_cast<int>(offset / height);

    return index >= 0 && index < static_cast<int>(rows_.size()) ? index : -1;
}

void gui_popup_menu_ctrl::on_mouse_pressed(int button, const Vector2 pt) {
    if (!open_) {
        open();

        return;
    }

    const auto index = row_at(pt);

    close();

    if (index >= 0) {
        select(index);
    }
}

void gui_popup_menu_ctrl::draw() const {
    const auto profile = get_profile();
    if (profile) {
        profile->draw_frame(get_bounds(), is_mouse_over(), is_disabled());
        profile->draw_text(get_text(), get_bounds(), alignment::left);
    }

    const auto rows = text_profile();
    if (!rows) {
        draw_children();

        return;
    }

    const auto bounds = get_bounds();
    const auto size = rows->get_font_size();
    const auto font = load_font(rows->get_font(), size);

    const Vector2 tip{bounds.x + bounds.width - arrow_width, bounds.y + (bounds.height / 2) + 3};
    DrawTriangle(
        {tip.x - (arrow_width / 2), tip.y - 5},
        {tip.x, tip.y},
        {tip.x + (arrow_width / 2), tip.y - 5},
        rows->font_color);

    if (!open_) {
        draw_children();

        return;
    }

    const auto height = row_height();
    const Rectangle popup{bounds.x, bounds.y + bounds.height, bounds.width, popup_height()};

    DrawRectangleRec(popup, rows->fill_color);
    DrawRectangleLinesEx(popup, 1, rows->border_color);

    for (size_t i = 0; i < rows_.size(); ++i) {
        const auto y = popup.y + (static_cast<float>(i) * height);
        if (y + height > popup.y + popup.height) {
            break;
        }

        const auto highlighted = static_cast<int>(i) == selected_;
        if (highlighted) {
            DrawRectangleRec({popup.x, y, popup.width, height}, rows->fill_color_hl);
        }

        DrawTextEx(font, rows_[i].text.c_str(), {popup.x + text_padding, y}, size, 1,
            highlighted ? rows->font_color_hl : rows->font_color);
    }

    draw_children();
}
