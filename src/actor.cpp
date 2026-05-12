#include "actor.hpp"

#include "animation_manager.hpp"
#include "game.hpp"
#include "texture_manager.hpp"

#include <boost/algorithm/string.hpp>
#include <rlgl.h>

namespace {
    constexpr Rectangle get_terrain_rectangle(const terrain_effect_type type) {
        switch (type) {
        case terrain_effect_type::swamp:      return {0, 274, 32, 16};
        case terrain_effect_type::lava_swamp: return {32, 274, 32, 16};
        case terrain_effect_type::near_water: return {0, 306, 32, 16};
        case terrain_effect_type::near_lava:  return {32, 306, 32, 16};
        default:                              return {};
        }
    }
}

actor::actor() : sprites_(load_texture("sprites.png")) {
    set_animation("idle");
}

void actor::update(const float dt) {
    if (animation_ != nullptr) {
        animation_->update(dt, animation_state_);
    }

    if (terrain_effect_type_ != terrain_effect_type::none) {
        if (terrain_effect_type_ == terrain_effect_type::swamp ||
            terrain_effect_type_ == terrain_effect_type::lava_swamp) {
            if (velocity_.x == 0 && velocity_.y == 0) {
                return;
            }
        }

        terrain_effect_timer_ -= dt;
        if (terrain_effect_timer_ <= 0) {
            terrain_effect_timer_ = 0.1f;
            terrain_effect_frame_++;

            if (terrain_effect_frame_ > 1) {
                terrain_effect_frame_ = 0;
            }
        }
    }
}

void actor::draw() const {
    if (animation_ == nullptr) {
        return;
    }

    rlPushMatrix();
    rlTranslatef(-8, -16, 0);

    animation_->draw(
        position_.x,
        position_.y,
        dir_,
        animation_state_);

    rlPopMatrix();

    if (carried_object_ != carry_object_type::none && sprites_.id != 0) {
        Vector2 dest{position_.x, position_.y - 40};

        get_carried_destination_override(dest);

        DrawTextureRec(sprites_, carried_object_rectangle_, dest, WHITE);
    }

    draw_terrain();
}

auto actor::is_facing_wall() const -> bool {
    return on_wall(look_at(dir_));
}

void actor::set_animation(const std::string &name) {
    const auto animation_name = boost::to_lower_copy(name);

    if (animation_name == animation_name_) {
        return;
    }

    animation_name_ = animation_name;
    animation_ = load_animation(animation_name_);

    if (animation_ == nullptr) {
        return;
    }

    animation_state_.reset(0, animation_);
}

void actor::set_carried_object(const carry_object_type type) {
    carried_object_ = type;
    carried_object_rectangle_ = get_carry_object_rect(type);
}

void actor::set_terrain(const terrain_effect_type type) {
    terrain_effect_type_ = type;
    terrain_effect_rect_ = get_terrain_rectangle(type);
}

void actor::draw_terrain() const {
    if (terrain_effect_type_ == terrain_effect_type::none) {
        return;
    }

    auto rectangle = terrain_effect_rect_;

    rectangle.y += static_cast<float>(terrain_effect_frame_) * 16;

    const auto [x, y] = get_position();
    const auto dx = x;
    const auto dy = y + 16;

    DrawTextureRec(sprites_, rectangle, {dx, dy}, WHITE);
}
