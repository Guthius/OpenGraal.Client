#include "gui_slider_ctrl.hpp"

#include "../dev_input.hpp"

#include <algorithm>

namespace {
    constexpr int sprite_track_left = 0;
    constexpr int sprite_track_middle = 1;
    constexpr int sprite_track_right = 2;
    constexpr int sprite_thumb = 3;
    constexpr int sprite_thumb_hl = 4;

    constexpr float fallback_thumb_width = 8.0f;
    constexpr float fallback_track_height = 4.0f;
}

void gui_slider_ctrl::set_value(const float value) {
    value_ = std::clamp(value, std::min(minimum_, maximum_), std::max(minimum_, maximum_));
}

void gui_slider_ctrl::set_range(const float minimum, const float maximum) {
    minimum_ = minimum;
    maximum_ = maximum;

    set_value(value_);
}

void gui_slider_ctrl::track(const Vector2 pt) {
    const auto bounds = get_bounds();
    if (bounds.width <= 0.0f) {
        return;
    }

    const auto fraction = std::clamp(pt.x / bounds.width, 0.0f, 1.0f);
    const auto previous = value_;

    set_value(minimum_ + ((maximum_ - minimum_) * fraction));

    if (value_ != previous && clicked) {
        clicked();
    }
}

void gui_slider_ctrl::on_mouse_pressed(int button, const Vector2 pt) {
    dragging_ = true;

    capture_mouse();
    track(pt);
}

void gui_slider_ctrl::on_mouse_move(const Vector2 pt) {
    if (!dragging_) {
        return;
    }

    if (!dev_input::mouse_down(MOUSE_BUTTON_LEFT)) {
        dragging_ = false;

        release_mouse();

        return;
    }

    track(pt);
}

void gui_slider_ctrl::on_mouse_released(int button, const Vector2 pt) {
    if (dragging_) {
        dragging_ = false;

        release_mouse();
        track(pt);
    }
}

void gui_slider_ctrl::draw() const {
    const auto profile = get_profile();
    if (!profile) {
        return;
    }

    const auto bounds = get_bounds();
    const auto span = maximum_ - minimum_;
    const auto fraction = span == 0.0f ? 0.0f : std::clamp((value_ - minimum_) / span, 0.0f, 1.0f);

    const auto texture = profile->get_texture();
    const auto left = profile->get_sprite_rect(sprite_track_left);
    const auto middle = profile->get_sprite_rect(sprite_track_middle);
    const auto right = profile->get_sprite_rect(sprite_track_right);
    const auto thumb = profile->get_sprite_rect(is_mouse_over() ? sprite_thumb_hl : sprite_thumb);

    if (!IsTextureValid(texture) || thumb.width <= 0) {
        const auto track = Rectangle{
            bounds.x,
            bounds.y + ((bounds.height - fallback_track_height) / 2.0f),
            bounds.width,
            fallback_track_height};

        DrawRectangleRec(track, profile->fill_color);
        DrawRectangleRec(
            {bounds.x + ((bounds.width - fallback_thumb_width) * fraction), bounds.y, fallback_thumb_width, bounds.height},
            is_mouse_over() ? profile->fill_color_hl : profile->fill_color);

        draw_children();

        return;
    }

    const auto track_y = bounds.y + ((bounds.height - left.height) / 2.0f);
    const auto middle_width = std::max(0.0f, bounds.width - left.width - right.width);

    DrawTextureRec(texture, left, {bounds.x, track_y}, WHITE);
    DrawTexturePro(
        texture,
        middle,
        {bounds.x + left.width, track_y, middle_width, middle.height},
        {0, 0},
        0.0f,
        WHITE);
    DrawTextureRec(texture, right, {bounds.x + left.width + middle_width, track_y}, WHITE);

    DrawTextureRec(
        texture,
        thumb,
        {bounds.x + ((bounds.width - thumb.width) * fraction), bounds.y + ((bounds.height - thumb.height) / 2.0f)},
        WHITE);

    draw_children();
}
