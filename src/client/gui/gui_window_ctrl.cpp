#include "gui_window_ctrl.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <raylib.h>

namespace {
    nine_patch state_normal{12, 14, 13, 18, 23, 19, 20, 21, 22};
    nine_patch state_disabled{15, 17, 16, 18, 23, 19, 20, 21, 22};

    constexpr float button_y = 5.0f;
    constexpr float button_width = 15.0f;
    constexpr float button_height = 14.0f;
    constexpr float button_right_inset = 38.0f;
    constexpr float button_stride = 17.0f;
    constexpr double double_click_seconds = 0.35;
    constexpr float maximize_seconds = 0.15f;

    constexpr std::array sprites_close{0, 1, 2};
    constexpr std::array sprites_maximize{3, 4, 5};
    constexpr std::array sprites_restore{6, 7, 8};

    auto get_title_bar_height(const gui_control_profile *profile, const nine_patch &patch) -> float {
        if (profile == nullptr) {
            return 0.0f;
        }

        return profile->get_sprite_rect(patch.t).height;
    }
}

auto gui_window_ctrl::title_buttons() const -> std::vector<std::pair<title_button, Rectangle>> {
    std::vector<std::pair<title_button, Rectangle>> buttons;

    auto x = get_size().x - button_right_inset;

    if (can_close_) {
        buttons.emplace_back(title_button::close, Rectangle{x, button_y, button_width, button_height});
        x -= button_stride;
    }

    if (can_maximize_) {
        buttons.emplace_back(title_button::maximize, Rectangle{x, button_y, button_width, button_height});
    }

    return buttons;
}

auto gui_window_ctrl::button_at(const Vector2 pt) const -> int {
    for (const auto &[kind, rect] : title_buttons()) {
        if (CheckCollisionPointRec(pt, rect)) {
            return static_cast<int>(kind);
        }
    }

    return -1;
}

void gui_window_ctrl::set_maximized(const bool maximized) {
    if (maximized == maximized_) {
        return;
    }

    if (maximized) {
        restore_bounds_ = get_bounds();

        const auto &parent = get_parent();
        const auto size = parent
                              ? parent->get_size()
                              : Vector2{static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};

        anim_to_ = {0, 0, size.x, size.y};
    } else {
        anim_to_ = restore_bounds_;
    }

    anim_from_ = get_bounds();
    anim_t_ = 0.0f;
    animating_ = true;
    maximized_ = maximized;
}

void gui_window_ctrl::update(const float dt) {
    gui_text_ctrl::update(dt);

    if (!animating_) {
        return;
    }

    anim_t_ = std::min(1.0f, anim_t_ + (dt / maximize_seconds));

    const auto ease = anim_t_ * anim_t_ * (3.0f - (2.0f * anim_t_));
    const auto at = [&](const float from, const float to) { return std::round(from + ((to - from) * ease)); };

    set_position({at(anim_from_.x, anim_to_.x), at(anim_from_.y, anim_to_.y)});
    set_size({at(anim_from_.width, anim_to_.width), at(anim_from_.height, anim_to_.height)});

    if (anim_t_ >= 1.0f) {
        animating_ = false;
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
            .x = bounds.x + 15,
            .y = bounds.y,
            .width = bounds.width - 20,
            .height = title_bar_sprite.height};

        profile->draw_text(get_text(), text_rect, alignment::left);

        for (const auto &[kind, rect] : title_buttons()) {
            const auto &sprites = kind == title_button::close ? sprites_close
                                  : maximized_                ? sprites_restore
                                                              : sprites_maximize;
            const auto state = is_disabled() ? 2 : hover_button_ == static_cast<int>(kind) ? 1
                                                                                           : 0;

            profile->draw_sprite(sprites[state], {bounds.x + rect.x, bounds.y + rect.y});
        }
    }

    draw_children();
}

void gui_window_ctrl::on_mouse_pressed(const int button, const Vector2 pt) {
    if (button == MOUSE_BUTTON_LEFT && !is_disabled()) {
        if (const auto hit = button_at(pt); hit >= 0) {
            bring_to_front();

            if (hit == static_cast<int>(title_button::close)) {
                if (close_requested) {
                    close_requested();
                } else {
                    set_visible(false);
                }
            } else {
                set_maximized(!maximized_);
            }

            return;
        }
    }

    if (!maximized_ && !animating_) {
        gui_control::on_mouse_pressed(button, pt);
        if (is_resizing()) {
            bring_to_front();

            return;
        }
    }

    if (button != MOUSE_BUTTON_LEFT || is_disabled()) {
        return;
    }

    auto *const profile = get_profile().get();
    const auto &patch = state_normal;

    if (const float title_bar_height = get_title_bar_height(profile, patch); pt.y < title_bar_height) {
        if (can_maximize_ && GetTime() - last_title_press_ < double_click_seconds) {
            last_title_press_ = 0.0;

            set_maximized(!maximized_);

            return;
        }

        last_title_press_ = GetTime();

        if (!draggable_ || maximized_ || animating_) {
            return;
        }

        dragging_ = true;
        drag_offset_ = pt;

        capture_mouse();

        bring_to_front();
    }
}

void gui_window_ctrl::on_mouse_released(const int button, const Vector2 pt) {
    gui_control::on_mouse_released(button, pt);

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
        hover_button_ = is_disabled() ? -1 : button_at(pt);

        if (!maximized_) {
            gui_control::on_mouse_move(pt);
        }

        return;
    }

    const auto [x, y] = local_to_global_coord(pt);

    set_position(Vector2{
        std::round(x - drag_offset_.x),
        std::round(y - drag_offset_.y)});
}

void gui_window_ctrl::on_mouse_exit() {
    hover_button_ = -1;

    gui_control::on_mouse_exit();
}
