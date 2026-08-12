#include "gui_scrollbar_ctrl.hpp"

#include <algorithm>

namespace {
    constexpr float minimum_thumb = 14.0f;
    constexpr float button_step = 16.0f;
    
    struct part {
        int normal, hot, disabled;

        [[nodiscard]] constexpr auto pick(const bool hot_now, const bool disabled_now) const -> int {
            return disabled_now ? disabled : hot_now ? hot
                                                     : normal;
        }
    };

    constexpr part up_button{1, 0, 2};
    constexpr part down_button{4, 3, 5};
    constexpr part left_button{19, 18, 20};
    constexpr part right_button{22, 21, 23};
    constexpr part v_thumb_top{6, 7, 8};
    constexpr part v_thumb_middle{9, 10, 11};
    constexpr part v_thumb_bottom{12, 13, 14};
    constexpr part v_track{15, 15, 17};
    constexpr part h_thumb_left{24, 25, 26};
    constexpr part h_thumb_middle{27, 28, 29};
    constexpr part h_thumb_right{30, 31, 32};
    constexpr part h_track{33, 33, 35};
}

void gui_scrollbar_ctrl::set_range(const float content, const float view) {
    content_ = std::max(content, 1.0f);
    view_ = std::max(view, 1.0f);

    set_value(value_);
}

void gui_scrollbar_ctrl::set_value(const float value) {
    value_ = std::clamp(value, 0.0f, max_value());
}

auto gui_scrollbar_ctrl::max_value() const -> float {
    return std::max(0.0f, content_ - view_);
}

auto gui_scrollbar_ctrl::button_length() const -> float {
    const auto profile = get_profile();
    if (!profile) {
        return 0.0f;
    }

    const auto sprite = profile->get_sprite_rect(vertical_ ? up_button.normal : left_button.normal);

    return vertical_ ? sprite.height : sprite.width;
}

auto gui_scrollbar_ctrl::track_length() const -> float {
    const auto bounds = get_bounds();

    return std::max(0.0f, (vertical_ ? bounds.height : bounds.width) - (button_length() * 2.0f));
}

auto gui_scrollbar_ctrl::thumb_length() const -> float {
    return std::max(minimum_thumb, track_length() * std::min(1.0f, view_ / content_));
}

auto gui_scrollbar_ctrl::thumb_offset() const -> float {
    const auto range = max_value();
    if (range <= 0.0f) {
        return 0.0f;
    }

    return (track_length() - thumb_length()) * (value_ / range);
}

void gui_scrollbar_ctrl::scroll_to(const float value) {
    const auto previous = value_;

    set_value(value);

    if (value_ != previous && changed) {
        changed(value_);
    }
}

void gui_scrollbar_ctrl::on_mouse_pressed(const int button, const Vector2 pt) {
    if (button != MOUSE_BUTTON_LEFT || max_value() <= 0.0f) {
        return;
    }

    const auto along = vertical_ ? pt.y : pt.x;
    const auto buttons = button_length();
    const auto full = vertical_ ? get_bounds().height : get_bounds().width;

    if (along < buttons) {
        scroll_to(value_ - button_step);

        return;
    }

    if (along >= full - buttons) {
        scroll_to(value_ + button_step);

        return;
    }

    const auto track_along = along - buttons;
    const auto offset = thumb_offset();

    if (track_along >= offset && track_along < offset + thumb_length()) {
        dragging_ = true;
        drag_grab_ = track_along - offset;

        capture_mouse();

        return;
    }

    scroll_to(track_along < offset ? value_ - (view_ * 0.8f) : value_ + (view_ * 0.8f));
}

void gui_scrollbar_ctrl::on_mouse_move(const Vector2 pt) {
    hover_along_ = vertical_ ? pt.y : pt.x;

    if (!dragging_) {
        return;
    }

    const auto span = track_length() - thumb_length();
    if (span <= 0.0f) {
        return;
    }

    const auto along = (vertical_ ? pt.y : pt.x) - button_length() - drag_grab_;

    scroll_to((along / span) * max_value());
}

void gui_scrollbar_ctrl::on_mouse_released(const int /*button*/, const Vector2 /*pt*/) {
    if (dragging_) {
        dragging_ = false;

        release_mouse();
    }
}

void gui_scrollbar_ctrl::on_mouse_exit() {
    hover_along_ = -1.0f;

    gui_control::on_mouse_exit();
}

void gui_scrollbar_ctrl::draw() const {
    const auto profile = get_profile();
    if (!profile) {
        return;
    }

    const auto bounds = get_bounds();
    const auto texture = profile->get_texture();
    const auto buttons = button_length();

    if (!IsTextureValid(texture) || buttons <= 0.0f) {
        DrawRectangleRec(bounds, profile->fill_color);

        const auto thumb = vertical_
                               ? Rectangle{bounds.x + 1, bounds.y + thumb_offset(), bounds.width - 2, thumb_length()}
                               : Rectangle{bounds.x + thumb_offset(), bounds.y + 1, thumb_length(), bounds.height - 2};

        DrawRectangleRec(thumb, is_mouse_over() ? profile->fill_color_hl : profile->border_color);
        DrawRectangleLinesEx(thumb, 1, profile->border_color_na);

        draw_children();

        return;
    }

    const auto stuck = max_value() <= 0.0f;
    const auto full = vertical_ ? bounds.height : bounds.width;
    const auto sprite = [&](const part &piece, const bool hot) { return profile->get_sprite_rect(piece.pick(hot, stuck)); };
    const auto stretch = [&](const Rectangle source, const Rectangle destination) {
        DrawTexturePro(texture, source, destination, {0, 0}, 0.0f, WHITE);
    };

    const auto hover_near = is_mouse_over() && !stuck ? hover_along_ : -1.0f;
    const auto near_start = hover_near >= 0.0f && hover_near < buttons;
    const auto near_end = hover_near >= full - buttons;
    const auto track_along = hover_near - buttons;
    const auto on_thumb = hover_near >= 0.0f && !near_start && !near_end &&
                          track_along >= thumb_offset() && track_along < thumb_offset() + thumb_length();

    const auto thumb = thumb_length();
    const auto offset = thumb_offset();

    if (vertical_) {
        stretch(sprite(v_track, false), {bounds.x, bounds.y + buttons, bounds.width, full - (buttons * 2.0f)});
        stretch(sprite(up_button, near_start), {bounds.x, bounds.y, bounds.width, buttons});
        stretch(sprite(down_button, near_end), {bounds.x, bounds.y + full - buttons, bounds.width, buttons});

        if (!stuck) {
            const auto top = sprite(v_thumb_top, on_thumb);
            const auto bottom = sprite(v_thumb_bottom, on_thumb);
            const auto y = bounds.y + buttons + offset;

            stretch(top, {bounds.x, y, bounds.width, top.height});
            stretch(sprite(v_thumb_middle, on_thumb),
                {bounds.x, y + top.height, bounds.width, thumb - top.height - bottom.height});
            stretch(bottom, {bounds.x, y + thumb - bottom.height, bounds.width, bottom.height});
        }
    } else {
        stretch(sprite(h_track, false), {bounds.x + buttons, bounds.y, full - (buttons * 2.0f), bounds.height});
        stretch(sprite(left_button, near_start), {bounds.x, bounds.y, buttons, bounds.height});
        stretch(sprite(right_button, near_end), {bounds.x + full - buttons, bounds.y, buttons, bounds.height});

        if (!stuck) {
            const auto left = sprite(h_thumb_left, on_thumb);
            const auto right = sprite(h_thumb_right, on_thumb);
            const auto x = bounds.x + buttons + offset;

            stretch(left, {x, bounds.y, left.width, bounds.height});
            stretch(sprite(h_thumb_middle, on_thumb),
                {x + left.width, bounds.y, thumb - left.width - right.width, bounds.height});
            stretch(right, {x + thumb - right.width, bounds.y, right.width, bounds.height});
        }
    }

    draw_children();
}
