#include "gui_text_edit_ctrl.hpp"

void gui_text_edit_ctrl::set_selection(const size_t start, const size_t length) {
    const auto text = get_text();
    const auto text_len = text.size();

    selection_anchor_ = std::clamp<size_t>(start, 0, text_len);
    caret_index_ = std::clamp<size_t>(start + length, 0, text_len);

    ensure_caret_visible();
}

void gui_text_edit_ctrl::update(const float dt) {
    gui_control::update(dt);

    if (!focused_ || is_disabled()) {
        return;
    }

    handle_keyboard_input();
    ensure_caret_visible();
}

void gui_text_edit_ctrl::draw() const {
    const auto profile = get_profile();
    if (!profile) {
        return;
    }

    const auto bounds = get_bounds();

    profile->draw_frame(bounds, is_mouse_over(), is_disabled());

    constexpr auto padding = k_text_padding;

    const auto display_text = get_display_text();
    const auto font = load_font(profile->get_font(), profile->get_font_size());
    if (!IsFontValid(font)) {
        profile->draw_text(display_text, bounds, alignment::left);
        return;
    }

    const auto [x, y] = local_to_global_coord({0.0f, 0.0f});

    const auto clip_x = static_cast<int>(x + padding);
    const auto clip_y = static_cast<int>(y);
    const auto clip_w = static_cast<int>(bounds.width - padding * 2.0f);
    const auto clip_h = static_cast<int>(bounds.height);

    BeginScissorMode(
        clip_x, clip_y,
        std::max(0, clip_w),
        std::max(0, clip_h));

    draw_selection(background_color_, font, display_text, bounds, padding);

    Rectangle text_rect = bounds;

    text_rect.x += padding - scroll_offset_;
    text_rect.width -= padding * 2.0f;

    profile->draw_text(display_text, text_rect, alignment::left);

    if (focused_) {
        draw_caret(caret_color_, font, display_text, bounds, padding);
    }

    EndScissorMode();
}

void gui_text_edit_ctrl::on_mouse_pressed(const int button, const Vector2 pt) {
    if (button != MOUSE_BUTTON_LEFT || is_disabled()) {
        return;
    }

    focused_ = true;
    selecting_ = true;

    capture_mouse();

    const auto index = index_from_x(pt.x);

    selection_anchor_ = index;
    caret_index_ = index;

    ensure_caret_visible();
}

void gui_text_edit_ctrl::on_mouse_move(const Vector2 pt) {
    if (!selecting_ || is_disabled()) {
        return;
    }

    caret_index_ = index_from_x(pt.x);
    ensure_caret_visible();
}

void gui_text_edit_ctrl::on_mouse_released(const int button, Vector2 pt) {
    if (button != MOUSE_BUTTON_LEFT) {
        return;
    }

    selecting_ = false;
    release_mouse();
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

auto gui_text_edit_ctrl::text_width(const Font &font, const std::string &text) -> float {
    if (!IsFontValid(font) || text.empty()) {
        return 0.0f;
    }

    return MeasureTextEx(font, text.c_str(), static_cast<float>(font.baseSize), 1.0f).x;
}

auto gui_text_edit_ctrl::text_width_to_index(const Font &font, const std::string &text, const size_t index) -> float {
    if (index == 0) {
        return 0.0f;
    }

    if (index >= text.size()) {
        return text_width(font, text);
    }

    return text_width(font, text.substr(0, index));
}
