#include "gui_check_box_ctrl.hpp"

namespace {
    constexpr int padding = 3;
    constexpr int sprite_normal_unchecked = 0;
    constexpr int sprite_normal_checked = 1;
    constexpr int sprite_mouse_over_unchecked = 2;
    constexpr int sprite_mouse_over_checked = 3;
}

void gui_check_box_ctrl::draw() const {
    auto get_sprite = [&]() -> int {
        if (is_mouse_over()) {
            return is_checked() ? sprite_mouse_over_checked : sprite_mouse_over_unchecked;
        }

        return is_checked() ? sprite_normal_checked : sprite_normal_unchecked;
    };

    if (const auto profile = get_profile(); profile) {
        const auto [x, y, width, height] = get_bounds();
        const auto sprite_rect = profile->get_sprite_rect(get_sprite());

        profile->draw_sprite(get_sprite(), Vector2(x, y));

        const auto text_rect = Rectangle{
            .x = x + sprite_rect.width + padding,
            .y = y,
            .width = width - sprite_rect.width - padding,
            .height = sprite_rect.height};

        profile->draw_text(get_text(), text_rect, alignment::left);
    }
}

void gui_check_box_ctrl::on_clicked() {
    set_checked(!is_checked());
}
