#include "thrown_item.hpp"

#include "carry_object.hpp"
#include "texture_manager.hpp"

#include <raymath.h>

namespace {
    constexpr auto carry_height = 40.0f;
    constexpr auto travel_distance_in_tiles = 7.0f;
    constexpr auto travel_distance = travel_distance_in_tiles * 16.0f;
}

thrown_item::thrown_item(const carry_object_type type, const Vector2 origin, const direction dir)
    : type_(type), start_(origin),
      end_(origin + get_direction_vector(dir) * travel_distance),
      position_(origin), dir_(dir),
      texture_(load_texture("sprites.png")),
      texture_rect_(get_carry_object_rect(type)),
      leap_type_(get_carry_object_leap_type(type)) {
    if (dir == direction::down) {
        end_.y += carry_height;
    }
}

void thrown_item::update(const float dt) {
    if (!alive_) {
        return;
    }

    time_elapsed_ += dt / duration_;
    if (time_elapsed_ >= 1.0f) {
        time_elapsed_ = 1.0f;
        alive_ = false;
    }

    position_.x = start_.x + (end_.x - start_.x) * time_elapsed_;
    position_.y = start_.y + (end_.y - start_.y) * time_elapsed_;

    if (dir_ == direction::left || dir_ == direction::right) {
        const float drop = carry_height * (time_elapsed_ * time_elapsed_);

        position_.y += drop;
    }
}

void thrown_item::draw() const {
    if (!is_alive()) {
        return;
    }

    if (texture_rect_.width == 0.0f || texture_rect_.height == 0.0f) {
        return;
    }

    draw_shadow();

    DrawTextureRec(texture_, texture_rect_, position_, WHITE);
}

void thrown_item::draw_shadow() const {
    const auto sx = position_.x + 4;

    float sy = 0.0f;
    switch (dir_) {
    case direction::left:
    case direction::right:
        sy = start_.y + carry_height + 16;
        break;

    case direction::up:
    case direction::down:
        {
            const auto shadow_start = start_.y + carry_height + 16.0f;
            const auto shadow_end = end_.y + 16.0f;
            const auto time = std::min<float>(time_elapsed_ * 1.0f, 1.0f);
            sy = shadow_start + (shadow_end - shadow_start) * time;
            break;
        }
    }

    DrawTextureRec(texture_, {0, 0, 24, 12}, {sx, sy}, WHITE);
}
