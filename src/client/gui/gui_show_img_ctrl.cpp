#include "gui_show_img_ctrl.hpp"

#include "../animation_manager.hpp"
#include "../texture_manager.hpp"

void gui_show_img_ctrl::set_ani(const std::string &ani) {
    if (ani == ani_) {
        return;
    }

    ani_ = ani;
    animation_ = load_animation(ani_);

    if (animation_ != nullptr) {
        state_.reset(0, animation_);
    }
}

void gui_show_img_ctrl::update(const float dt) {
    if (animation_ == nullptr && !ani_.empty()) {
        animation_ = load_animation(ani_);
        if (animation_ != nullptr) {
            state_.reset(0, animation_);
        }
    }

    if (animation_ != nullptr) {
        animation_->update(dt, state_);
    }

    gui_control::update(dt);
}

void gui_show_img_ctrl::draw() const {
    const auto bounds = get_bounds();
    const auto x = bounds.x + offset_.x;
    const auto y = bounds.y + offset_.y;

    if (animation_ != nullptr) {
        animation_->draw(x, y, dir_, state_);
    } else if (!image_.empty()) {
        if (const auto texture = load_texture(image_); IsTextureValid(texture)) {
            DrawTextureV(texture, {x, y}, WHITE);
        }
    }

    draw_children();
}
