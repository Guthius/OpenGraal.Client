#include "gui_text_edit_ctrl.hpp"

#include "../dev_input.hpp"
#include "../font_manager.hpp"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float text_padding = 4.0f;
    constexpr float min_caret_width = 1.0f;
    constexpr float selection_padding = 2.0f;
    constexpr float caret_blink_period = 1.0f;
    constexpr float dropdown_arrow_width = 12.0f;
    constexpr float dropdown_row_padding = 2.0f;
    constexpr float dropdown_max_height = 200.0f;

    nine_patch background{0, 1, 2, 3, -1, 4, 5, 6, 7};
}

void gui_text_edit_ctrl::set_selection(const size_t start, const size_t length) {
    const auto text = get_text();
    const auto text_len = text.size();

    selection_anchor_ = std::clamp<size_t>(start, 0, text_len);
    caret_index_ = std::clamp<size_t>(start + length, 0, text_len);

    ensure_caret_visible();
}

void gui_text_edit_ctrl::update(const float dt) {
    gui_control::update(dt);

    if (!has_focus() || is_disabled()) {
        caret_timer_ = 0.0f;

        return;
    }

    caret_timer_ += dt;

    handle_keyboard_input();
    ensure_caret_visible();

    if (caret_index_ != last_caret_index_) {
        last_caret_index_ = caret_index_;
        caret_timer_ = 0.0f;
    }
}

void gui_text_edit_ctrl::on_focus() {
    caret_timer_ = 0.0f;
    last_caret_index_ = caret_index_;
}

void gui_text_edit_ctrl::draw() const {
    const auto profile = get_profile();
    if (!profile) {
        return;
    }

    const auto bounds = get_bounds();

    profile->draw_frame(bounds, false, is_disabled());
    profile->draw_nine_patch(bounds, background);

    const auto display_text = get_display_text();
    const auto size = profile->get_font_size();
    const auto font = load_font(profile->get_font(), size);
    if (!IsFontValid(font)) {
        profile->draw_text(display_text, bounds, alignment::left);
        return;
    }

    const auto [x, y] = local_to_global_coord({0.0f, 0.0f});

    const auto clip_x = static_cast<int>(x + text_padding);
    const auto clip_y = static_cast<int>(y);
    const auto clip_w = std::max(0, static_cast<int>(bounds.width - (text_padding * 2.0f)));
    const auto clip_h = std::max(0, static_cast<int>(bounds.height));

    BeginScissorMode(clip_x, clip_y, clip_w, clip_h);

    draw_selection(profile->fill_color_hl, font, size, display_text, bounds, text_padding);

    Rectangle text_rect = bounds;

    text_rect.x += text_padding - scroll_offset_;
    text_rect.width -= text_padding * 2.0f;

    profile->draw_text(display_text, text_rect, alignment::left);

    if (const auto start = get_selection_start(), end = get_selection_end(); end > start) {
        const auto from = text_width_to_index(font, size, display_text, start);
        const auto to = text_width_to_index(font, size, display_text, end);

        const auto selection_x = static_cast<int>(x + text_padding + from - scroll_offset_);
        const auto selection_w = static_cast<int>(to - from);

        const auto left = std::max(clip_x, selection_x);
        const auto right = std::min(clip_x + clip_w, selection_x + selection_w);

        if (right > left) {
            BeginScissorMode(left, clip_y, right - left, clip_h);
            profile->draw_text(display_text, text_rect, alignment::left, profile->font_color_sel);
            BeginScissorMode(clip_x, clip_y, clip_w, clip_h);
        }
    }

    if (has_focus() && std::fmod(caret_timer_, caret_blink_period) < (caret_blink_period / 2.0f)) {
        draw_caret(profile->caret_color, font, size, display_text, bounds, text_padding);
    }

    EndScissorMode();

    if (dropdown_rows_.empty()) {
        return;
    }

    const Vector2 tip{bounds.x + bounds.width - dropdown_arrow_width, bounds.y + (bounds.height / 2.0f) + 3.0f};
    DrawTriangle(
        {tip.x - (dropdown_arrow_width / 2.0f), tip.y - 5.0f},
        {tip.x, tip.y},
        {tip.x + (dropdown_arrow_width / 2.0f), tip.y - 5.0f},
        profile->font_color);

    if (!dropdown_open_) {
        return;
    }

    const auto popup = dropdown_bounds();
    const auto row = profile->get_font_size() + dropdown_row_padding;

    DrawRectangleRec(popup, profile->fill_color);
    DrawRectangleLinesEx(popup, 1, profile->border_color);

    for (size_t i = 0; i < dropdown_rows_.size(); ++i) {
        const auto row_y = popup.y + (static_cast<float>(i) * row);
        if (row_y + row > popup.y + popup.height) {
            break;
        }

        DrawTextEx(font, dropdown_rows_[i].c_str(), {popup.x + text_padding, row_y},
            profile->get_font_size(), 1, profile->font_color);
    }
}

void gui_text_edit_ctrl::on_mouse_pressed(const int button, const Vector2 pt) {
    if (button != MOUSE_BUTTON_LEFT || is_disabled()) {
        return;
    }

    if (dropdown_open_) {
        const auto row = dropdown_row_at(pt);

        close_dropdown();

        if (row >= 0) {
            set_text(dropdown_rows_[static_cast<size_t>(row)]);

            caret_index_ = get_text().size();
            selection_anchor_ = caret_index_;
        }

        return;
    }

    if (!dropdown_rows_.empty() && pt.x >= get_bounds().width - arrow_width()) {
        focus();
        open_dropdown();

        return;
    }

    focus();
    selecting_ = true;

    capture_mouse();

    const auto index = index_from_x(pt.x);

    selection_anchor_ = index;
    caret_index_ = index;

    ensure_caret_visible();
}

void gui_text_edit_ctrl::on_mouse_move(const Vector2 pt) {
    if (is_disabled()) {
        return;
    }

    if (!selecting_) {
        const auto ibeam = !dropdown_open_ &&
                           (dropdown_rows_.empty() || pt.x < get_bounds().width - arrow_width());

        SetMouseCursor(ibeam ? MOUSE_CURSOR_IBEAM : MOUSE_CURSOR_DEFAULT);

        return;
    }

    caret_index_ = index_from_x(pt.x);
    ensure_caret_visible();
}

void gui_text_edit_ctrl::on_mouse_released(const int button, Vector2 pt) {
    if (button != MOUSE_BUTTON_LEFT || dropdown_open_) {
        return;
    }

    selecting_ = false;
    release_mouse();
}

void gui_text_edit_ctrl::on_mouse_enter() {
    if (!is_disabled() && !dropdown_open_) {
        SetMouseCursor(MOUSE_CURSOR_IBEAM);
    }
}

void gui_text_edit_ctrl::on_mouse_exit() {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

void gui_text_edit_ctrl::on_blur() {
    close_dropdown();
}

void gui_text_edit_ctrl::set_dropdown_rows(std::vector<std::string> rows) {
    dropdown_rows_ = std::move(rows);

    if (dropdown_rows_.empty()) {
        close_dropdown();
    }
}

auto gui_text_edit_ctrl::arrow_width() const -> float {
    return dropdown_rows_.empty() ? 0.0f : dropdown_arrow_width;
}

void gui_text_edit_ctrl::open_dropdown() {
    if (dropdown_open_ || dropdown_rows_.empty()) {
        return;
    }

    dropdown_open_ = true;

    capture_mouse();
    bring_to_front();
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

void gui_text_edit_ctrl::close_dropdown() {
    if (!dropdown_open_) {
        return;
    }

    dropdown_open_ = false;

    release_mouse();
}

auto gui_text_edit_ctrl::dropdown_bounds() const -> Rectangle {
    const auto profile = get_profile();
    const auto row = (profile ? profile->get_font_size() : 12.0f) + dropdown_row_padding;
    const auto bounds = get_bounds();
    const auto height = std::min(dropdown_max_height, static_cast<float>(dropdown_rows_.size()) * row);

    return {bounds.x, bounds.y + bounds.height, bounds.width, height};
}

auto gui_text_edit_ctrl::dropdown_row_at(const Vector2 pt) const -> int {
    const auto profile = get_profile();
    const auto row = (profile ? profile->get_font_size() : 12.0f) + dropdown_row_padding;
    const auto bounds = get_bounds();

    if (!dropdown_open_ || row <= 0.0f || pt.x < 0.0f || pt.x >= bounds.width) {
        return -1;
    }

    const auto offset = pt.y - bounds.height;
    if (offset < 0.0f || offset >= dropdown_bounds().height) {
        return -1;
    }

    const auto index = static_cast<int>(offset / row);

    return index >= 0 && index < static_cast<int>(dropdown_rows_.size()) ? index : -1;
}

auto gui_text_edit_ctrl::get_display_text() const -> std::string {
    auto text = get_text();
    if (!password_) {
        return text;
    }

    return std::string(text.size(), '*');
}

auto gui_text_edit_ctrl::get_selection_start() const -> size_t {
    return std::min(selection_anchor_, caret_index_);
}

auto gui_text_edit_ctrl::get_selection_end() const -> size_t {
    return std::max(selection_anchor_, caret_index_);
}

auto gui_text_edit_ctrl::get_selection_length() const -> size_t {
    const auto start = get_selection_start();
    const auto end = get_selection_end();

    return end > start ? end - start : 0;
}

auto gui_text_edit_ctrl::text_width(const Font &font, const float size, const std::string &text) -> float {
    if (!IsFontValid(font) || text.empty()) {
        return 0.0f;
    }

    return MeasureTextEx(font, text.c_str(), size, 1.0f).x;
}

auto gui_text_edit_ctrl::text_width_to_index(const Font &font, const float size, const std::string &text, const size_t index) -> float {
    if (index == 0) {
        return 0.0f;
    }

    if (index >= text.size()) {
        return text_width(font, size, text);
    }

    return text_width(font, size, text.substr(0, index));
}

auto gui_text_edit_ctrl::index_from_x(float local_x) -> size_t {
    const auto profile = get_profile();
    if (!profile) {
        return 0;
    }

    const auto size = profile->get_font_size();
    const auto font = load_font(profile->get_font(), size);
    const auto text = get_display_text();

    caret_index_ = std::min(caret_index_, text.size());
    selection_anchor_ = std::min(selection_anchor_, text.size());

    if (!IsFontValid(font) || text.empty()) {
        return 0;
    }

    const auto x = local_x - text_padding + scroll_offset_;
    if (x <= 0.0f) {
        return 0;
    }

    auto left = 0.0f;
    for (size_t i = 0; i < text.size(); ++i) {
        const auto right = text_width_to_index(font, size, text, i + 1);
        if (x < (left + right) * 0.5f) {
            return i;
        }

        left = right;
    }

    return text.size();
}

void gui_text_edit_ctrl::delete_selection() {
    const auto start = get_selection_start();
    const auto end = get_selection_end();
    if (end <= start) {
        return;
    }

    auto text = get_text();
    text.erase(start, end - start);
    set_text(text);

    caret_index_ = start;
    selection_anchor_ = start;
}

void gui_text_edit_ctrl::insert_text(const std::string &insert) {
    if (insert.empty()) {
        return;
    }

    if (get_selection_length() > 0) {
        delete_selection();
    }

    auto text = get_text();

    auto room = insert;
    if (max_length_ > 0 && text.size() + room.size() > max_length_) {
        room.resize(max_length_ > text.size() ? max_length_ - text.size() : 0);
    }

    if (room.empty()) {
        return;
    }

    caret_index_ = std::min(caret_index_, text.size());
    text.insert(caret_index_, room);
    caret_index_ += room.size();
    selection_anchor_ = caret_index_;

    set_text(text);
}

void gui_text_edit_ctrl::handle_keyboard_input() {
    auto text = get_text();

    const auto shift = dev_input::key_down(KEY_LEFT_SHIFT) || dev_input::key_down(KEY_RIGHT_SHIFT);

    if (dev_input::key_pressed(KEY_ENTER) || dev_input::key_pressed(KEY_KP_ENTER)) {
        close_dropdown();

        if (submitted) {
            submitted();
        }

        return;
    }

    if (dropdown_open_ && dev_input::key_pressed(KEY_ESCAPE)) {
        close_dropdown();
        return;
    }

    if (dev_input::key_pressed(KEY_LEFT)) {
        if (caret_index_ > 0) {
            caret_index_--;
        }
        if (!shift) {
            selection_anchor_ = caret_index_;
        }
    }

    if (dev_input::key_pressed(KEY_RIGHT)) {
        if (caret_index_ < text.size()) {
            caret_index_++;
        }
        if (!shift) {
            selection_anchor_ = caret_index_;
        }
    }

    if (dev_input::key_pressed(KEY_HOME)) {
        caret_index_ = 0;
        if (!shift) {
            selection_anchor_ = caret_index_;
        }
    }

    if (dev_input::key_pressed(KEY_END)) {
        caret_index_ = text.size();
        if (!shift) {
            selection_anchor_ = caret_index_;
        }
    }

    if (read_only_) {
        return;
    }

    if (dev_input::key_pressed(KEY_BACKSPACE)) {
        if (get_selection_length() > 0) {
            delete_selection();
        } else if (caret_index_ > 0) {
            text.erase(caret_index_ - 1, 1);
            caret_index_--;
            selection_anchor_ = caret_index_;
            set_text(text);
        }
    }

    if (dev_input::key_pressed(KEY_DELETE)) {
        if (get_selection_length() > 0) {
            delete_selection();
        } else if (caret_index_ < text.size()) {
            text.erase(caret_index_, 1);
            selection_anchor_ = caret_index_;
            set_text(text);
        }
    }

    int key = dev_input::char_pressed();
    while (key > 0) {
        if (key >= 32 && key != 127) {
            const char c = static_cast<char>(key);
            insert_text(std::string(1, c));
        }
        key = dev_input::char_pressed();
    }
}

void gui_text_edit_ctrl::ensure_caret_visible() {
    const auto profile = get_profile();
    if (!profile) {
        return;
    }

    const auto size = profile->get_font_size();
    const auto font = load_font(profile->get_font(), size);
    if (!IsFontValid(font)) {
        return;
    }

    const auto text = get_display_text();
    const auto total_width = text_width(font, size, text);
    const auto bounds = get_bounds();
    const auto view_width = std::max(0.0f, bounds.width - (text_padding * 2.0f));

    if (total_width <= view_width) {
        scroll_offset_ = 0.0f;
        return;
    }

    const auto caret_x = text_width_to_index(font, size, text, caret_index_);
    if (caret_x - scroll_offset_ > view_width) {
        scroll_offset_ = caret_x - view_width;
    } else if (caret_x < scroll_offset_) {
        scroll_offset_ = caret_x;
    }

    const auto max_scroll = std::max(0.0f, total_width - view_width);

    scroll_offset_ = std::clamp(scroll_offset_, 0.0f, max_scroll);
}

auto gui_text_edit_ctrl::band_height(const Font &font, const float size, const std::string &text, const float bounds_height) -> float {
    const auto text_height = text.empty() ? size : MeasureTextEx(font, text.c_str(), size, 1.0f).y;

    return std::min(bounds_height, text_height + (selection_padding * 2.0f));
}

void gui_text_edit_ctrl::draw_selection(Color color, Font font, const float size, const std::string &text, Rectangle bounds, float padding) const {
    const auto start = get_selection_start();
    const auto end = get_selection_end();
    if (end <= start || !IsFontValid(font)) {
        return;
    }

    const auto start_x = text_width_to_index(font, size, text, start);
    const auto end_x = text_width_to_index(font, size, text, end);

    const auto draw_x = bounds.x + padding + start_x - scroll_offset_;
    const auto draw_w = end_x - start_x;

    if (draw_w <= 0.0f) {
        return;
    }

    const auto height = band_height(font, size, text, bounds.height);

    DrawRectangleRec(
        Rectangle{
            draw_x,
            std::round(bounds.y + ((bounds.height - height) / 2.0f)),
            draw_w,
            height},
        color);
}

void gui_text_edit_ctrl::draw_caret(Color color, Font font, const float size, const std::string &text, Rectangle bounds, float padding) const {
    if (!IsFontValid(font)) {
        return;
    }

    const auto caret_x = text_width_to_index(font, size, text, caret_index_);
    const auto draw_x = bounds.x + padding + caret_x - scroll_offset_;
    const auto height = band_height(font, size, text, bounds.height);

    DrawRectangleRec(
        Rectangle{
            draw_x,
            std::round(bounds.y + ((bounds.height - height) / 2.0f)),
            min_caret_width,
            height},
        color);
}
