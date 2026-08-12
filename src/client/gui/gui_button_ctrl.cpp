#include "gui_button_ctrl.hpp"

#include <cmath>

namespace {
    nine_patch state_normal{0, 1, 2, 3, 4, 5, 6, 7, 8};
    nine_patch state_mouse_over{9, 10, 11, 12, 13, 14, 15, 16, 17};
    nine_patch state_pressed{18, 19, 20, 21, 22, 23, 24, 25, 26};
    nine_patch state_disabled{27, 28, 29, 30, 31, 32, 33, 34, 35};

    constexpr float icon_margin = 6.0f;
}

auto gui_button_ctrl::get_icon() -> const std::shared_ptr<gui_drawing_panel> & {
    if (!icon_) {
        icon_ = std::make_shared<gui_drawing_panel>();
        icon_->set_size(icon_size_);

        add_child(icon_);
    }

    return icon_;
}

void gui_button_ctrl::set_icon_size(const float width, const float height) {
    icon_size_ = {width, height};

    if (icon_) {
        icon_->set_size(icon_size_);
    }
}

void gui_button_ctrl::update(const float dt) {
    gui_button_base_ctrl::update(dt);

    if (!icon_) {
        return;
    }

    const auto size = get_size();
    const auto x = get_text().empty() ? std::round((size.x - icon_size_.x) / 2.0f) : icon_margin;

    icon_->set_position({x, std::round((size.y - icon_size_.y) / 2.0f)});
    icon_->set_disabled(is_disabled());
}

void gui_button_ctrl::draw() const {
    auto get_state_nine_patch = [&]() -> const nine_patch & {
        if (is_disabled())
            return state_disabled;
        if (is_pressed())
            return state_pressed;
        if (is_mouse_over())
            return state_mouse_over;
        return state_normal;
    };

    if (const auto profile = get_profile(); profile) {
        profile->draw_nine_patch(get_bounds(), get_state_nine_patch());
        profile->draw_text(get_text(), get_bounds(), alignment::center);
    }

    draw_children();
}
