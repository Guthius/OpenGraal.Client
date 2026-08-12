#include "gui_text_list_ctrl.hpp"

#include "../dev_input.hpp"
#include "../font_manager.hpp"
#include "gui_utils.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
    constexpr float text_padding = 4.0f;
    constexpr float frame_padding = 3.0f;

    const nine_patch border{0, 1, 2, 3, -1, 4, 5, 6, 7};

    constexpr std::array default_tab_stops = {0.0f, 90.0f, 150.0f, 190.0f, 240.0f};

    auto split_columns(const std::string &text) -> std::vector<std::string> {
        std::vector<std::string> columns;

        size_t start = 0;
        while (start <= text.size()) {
            const auto tab = text.find('\t', start);

            columns.push_back(text.substr(start, tab == std::string::npos ? std::string::npos : tab - start));

            if (tab == std::string::npos) {
                break;
            }

            start = tab + 1;
        }

        return columns;
    }
}

auto gui_text_list_ctrl::get_selected_row() const -> int {
    return selected_ >= 0 && selected_ < static_cast<int>(rows_.size()) ? rows_[selected_].id : -1;
}

auto gui_text_list_ctrl::get_selected_text() const -> std::string {
    return selected_ >= 0 && selected_ < static_cast<int>(rows_.size()) ? rows_[selected_].text : std::string{};
}

void gui_text_list_ctrl::clear_rows() {
    rows_.clear();
    selected_ = -1;
    scroll_ = 0;
}

void gui_text_list_ctrl::add_row(const int id, const std::string &text) {
    rows_.push_back({.id = id, .text = text});
}

void gui_text_list_ctrl::remove_row(const int id) {
    const auto match = std::ranges::find_if(rows_, [id](const row &entry) { return entry.id == id; });
    if (match == rows_.end()) {
        return;
    }

    if (const auto index = static_cast<int>(std::distance(rows_.begin(), match)); index <= selected_) {
        selected_ = index == selected_ ? -1 : selected_ - 1;
    }

    rows_.erase(match);
}

void gui_text_list_ctrl::sort() {
    const auto chosen = get_selected_row();

    std::ranges::stable_sort(rows_, [](const row &a, const row &b) { return a.text < b.text; });

    selected_ = -1;
    for (size_t i = 0; i < rows_.size(); ++i) {
        if (rows_[i].id == chosen) {
            selected_ = static_cast<int>(i);
        }
    }
}

void gui_text_list_ctrl::set_selected_row(const int id) {
    for (size_t i = 0; i < rows_.size(); ++i) {
        if (rows_[i].id != id || static_cast<int>(i) == selected_) {
            continue;
        }

        selected_ = static_cast<int>(i);

        if (selected) {
            selected(rows_[i].id, rows_[i].text, selected_);
        }

        return;
    }
}

auto gui_text_list_ctrl::text_profile() const -> const std::shared_ptr<gui_control_profile> & {
    return text_profile_ ? text_profile_ : get_profile();
}

auto gui_text_list_ctrl::row_height() const -> float {
    return line_height_;
}

auto gui_text_list_ctrl::visible_rows() const -> int {
    const auto height = row_height();

    return height > 0.0f ? static_cast<int>(get_bounds().height / height) : 0;
}

void gui_text_list_ctrl::update(const float dt) {
    gui_control::update(dt);

    if (!is_mouse_over()) {
        return;
    }

    if (const auto wheel = dev_input::mouse_wheel(); wheel != 0.0f) {
        const auto limit = std::max(0, static_cast<int>(rows_.size()) - visible_rows());

        scroll_ = std::clamp(scroll_ - (static_cast<int>(wheel) * 3), 0, limit);
    }
}

auto gui_text_list_ctrl::row_at(const Vector2 pt) const -> int {
    if (!contains(pt) || row_height() <= 0.0f) {
        return -1;
    }

    const auto index = scroll_ + static_cast<int>(std::max(0.0f, pt.y - frame_padding) / row_height());

    return index >= 0 && index < static_cast<int>(rows_.size()) ? index : -1;
}

void gui_text_list_ctrl::on_mouse_pressed(int, const Vector2 pt) {
    if (const auto index = row_at(pt); index >= 0) {
        set_selected_row(rows_[index].id);
    }
}

void gui_text_list_ctrl::draw() const {
    const auto profile = get_profile();
    if (profile) {
        profile->draw_frame(get_bounds(), false, is_disabled());
        profile->draw_nine_patch(get_bounds(), border);
    }

    const auto rows = text_profile();
    if (!rows) {
        draw_children();

        return;
    }

    const auto bounds = get_bounds();
    const auto size = rows->get_font_size();
    const auto font = load_font(rows->get_font(), size);
    const auto height = row_height();

    const auto clip = local_to_global_coord({0, 0});
    BeginScissorMode(
        static_cast<int>(clip.x) + 2, static_cast<int>(clip.y) + 2,
        std::max(0, static_cast<int>(bounds.width) - 4), std::max(0, static_cast<int>(bounds.height) - 4));

    for (size_t i = static_cast<size_t>(scroll_); i < rows_.size(); ++i) {
        const auto y = bounds.y + frame_padding + (static_cast<float>(static_cast<int>(i) - scroll_) * height);
        if (y + height > bounds.y + bounds.height - frame_padding) {
            break;
        }

        const auto highlighted = static_cast<int>(i) == selected_;
        if (highlighted) {
            DrawRectangleRec({bounds.x + 2, y, bounds.width - 4, height}, rows->fill_color_hl);
        }

        const auto columns = split_columns(rows_[i].text);
        const auto color = highlighted ? rows->font_color_hl : rows->font_color;
        const auto text_y = y + std::round((height - size) / 2.0f);

        for (size_t column = 0; column < columns.size(); ++column) {
            const auto offset = columns_.empty()
                                    ? (column < default_tab_stops.size() ? default_tab_stops[column] : default_tab_stops.back())
                                    : (column < columns_.size() ? columns_[column] : columns_.back());

            DrawTextEx(font, columns[column].c_str(), {bounds.x + text_padding + offset, text_y}, size, 1, color);
        }
    }

    EndScissorMode();

    draw_children();
}
