#include "gui_window_ctrl.hpp"

#include <raylib.h>

namespace {
    nine_patch state_normal{12, 14, 13, 18, 23, 19, 20, 21, 22};
    nine_patch state_disabled{15, 17, 16, 18, 23, 19, 20, 21, 22};

    auto get_title_bar_height(const gui_control_profile *profile, const nine_patch &patch) -> float {
        if (!profile) {
            return 0.0f;
        }

        return profile->get_sprite_rect(patch.t).height;
    }
}

void gui_window_ctrl::draw() const {
    auto get_nine_patch = [&]() -> const nine_patch & {
        return is_disabled() ? state_disabled : state_normal;
    };

    if (const auto profile = get_profile(); profile) {
        const auto &bounds = get_bounds();
        const auto &nine_patch = get_nine_patch();
        const auto &title_bar_sprite = profile->get_sprite_rect(nine_patch.t);

        profile->draw_nine_patch(bounds, nine_patch);

        const auto text_rect = Rectangle{
            .x = bounds.x + 10,
            .y = bounds.y,
            .width = bounds.width - 20,
            .height = title_bar_sprite.height};

        profile->draw_text(get_text(), text_rect, alignment::left);
    }

    draw_children();
}

void gui_window_ctrl::on_mouse_pressed(const int button, const Vector2 pt) {
    if (button != MOUSE_BUTTON_LEFT || is_disabled()) {
        return;
    }

    if (!draggable_) {
        return;
    }

    const auto profile = get_profile().get();
    const auto &patch = state_normal;

    if (const float title_bar_height = get_title_bar_height(profile, patch); pt.y < title_bar_height) {
        dragging_ = true;
        drag_offset_ = pt;

        capture_mouse();

        bring_to_front();
    }
}

void gui_window_ctrl::on_mouse_released(const int button, const Vector2 pt) {
    if (button != MOUSE_BUTTON_LEFT) {
        return;
    }

    if (dragging_) {
        dragging_ = false;
        release_mouse();
    }
}

void gui_window_ctrl::on_mouse_move(const Vector2 pt) {
    if (!dragging_) {
        return;
    }

    const auto [x, y] = local_to_global_coord(pt);

    set_position(Vector2{
        x - drag_offset_.x,
        y - drag_offset_.y});
}
