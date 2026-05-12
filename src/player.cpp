#include "player.hpp"

#include "game.hpp"
#include "sound_manager.hpp"
#include "texture_manager.hpp"

#include <raymath.h>

static constexpr float jump_speed = 0.05f;
static constexpr Vector2 jump_frames[][8] =
    {
        {
         {0, -24},
         {0, -44},
         {0, -61},
         {0, -72},
         {0, -80},
         {0, -83},
         {0, -83},
         {0, -80},
         },
        {
         {-16, -3},
         {-32, -8},
         {-48, 0},
         {-61, 8},
         {-72, 18},
         {-83, 27},
         {-93, 35},
         {-104, 48},
         },
        {
         {0, -3},
         {0, 3},
         {0, 8},
         {0, 24},
         {0, 45},
         {0, 64},
         {0, 86},
         {0, 112},
         },
        {
         {16, -3},
         {32, -8},
         {48, 0},
         {61, 8},
         {72, 18},
         {83, 27},
         {93, 35},
         {104, 48},
         },
};

player::player()
    : sprites_(load_texture("sprites.png")),
      jump_sound_(load_sound("jump.wav")) {
}

void player::update(const float dt) {
    update_new(dt);

    // // Handle temporary lift-show state: show pull animation and item in front, block other actions
    // if (mode_ == player_state::lift_show)
    // {
    // 	lift_show_timer_ -= dt;
    // 	if (lift_show_timer_ <= 0.0f)
    // 	{
    // 		mode_ = player_state::idle;
    // 		UpdateAnimation();
    // 	}
    //
    // 	actor::update(dt);
    // 	UpdateOverlay(dt);
    // 	return;
    // }
    //
    // auto position = get_position();
    //
    // const auto mode = mode_;
    // const auto speed = GetSpeed(dt, speed_);
    //
    // if (const auto slide_speed = GetSpeed(dt, slide_speed_);
    // 	CheckJump(dt, position) ||
    // 	CheckMovement(position, speed, slide_speed))
    // {
    // 	set_position(position);
    // }
    //
    // CheckAttack(position);
    //
    // if (mode_ == player_state::walk)
    // {
    // 	if (CheckForSignAt(position))
    // 	{
    // 		mode_ = player_state::idle;
    // 	}
    // }
    //
    // CheckThrow();
    // CheckForLevelLinkAt(position);
    // CheckPushAndPull();
    //
    // if (mode_ != mode)
    // {
    // 	if (mode_ != player_state::idle && mode_ != player_state::walk && mode_ != player_state::lift_show)
    // 	{
    // 		DropCarriedItem();
    // 	}
    //
    // 	UpdateAnimation();
    // }
    //
    actor::update(dt);
}

auto player::check_for_level_link_at_at(const Vector2 &position) -> bool {
    const auto dir = get_direction();

    const auto [dirx, diry] = get_direction_vector(dir);
    const auto x = position.x + 16 + dirx * 17;
    const auto y = position.y + 16 + diry * 17;

    const auto level = get_current_level();
    if (level == nullptr) {
        return false;
    }

    const auto link = level->get_link_at(x, y);
    if (link == nullptr) {
        return false;
    }

    const auto dx = link->get_new_x() == "playerx" ? position.x : (std::stof(link->get_new_x()) + 0.5f) * 16;
    const auto dy = link->get_new_y() == "playery" ? position.y : (std::stof(link->get_new_y()) + 1.0f) * 16;

    TraceLog(LOG_INFO, "Warp to %s @ %s, %s (%f, %f)",
        link->get_new_level().c_str(),
        link->get_new_x().c_str(),
        link->get_new_y().c_str(),
        dx, dy);

    const auto pos = Vector2{dx, dy};

    set_position(pos);

    change_level(link->get_new_level());

    // Many levels will warp players partially on top of walls... nudge them off...
    try_move_from_wall(pos);

    return true;
}

auto player::check_for_sign_at(const Vector2 &position) const -> bool {
    if (!is_facing_wall()) {
        return false;
    }

    const auto dir = get_direction();

    const auto [dirx, diry] = get_direction_vector(dir);
    const auto x = position.x + 16 + dirx * 24;
    const auto y = position.y + 16 + diry * 24;

    const auto level = get_current_level();
    if (level == nullptr) {
        return false;
    }

    const auto sign = level->get_sign_at(x, y);
    if (sign == nullptr) {
        return false;
    }

    show_sign(sign->get_text());

    return true;
}

void player::try_destroy_object_facing(const Vector2 &position) const {
    auto [ax, ay] = position + Vector2(16, 16) + get_direction_vector(get_direction()) * 32;

    const auto [leap_type, leap_x, leap_y] = get_current_level()->try_destroy_object_at(ax, ay);
    if (leap_type != leap_effect_type::none) {
        spawn_leaps(leap_type, Vector2(
                                   static_cast<float>(leap_x),
                                   static_cast<float>(leap_y)));
    }
}

auto player::try_move_from_wall(Vector2 position) -> void {
    auto collides = [&](const Vector2 &p) {
        return on_wall(Rectangle{p.x, p.y, 31.0f, 31.0f});
    };

    if (collides(position)) {
        static constexpr Vector2 search_dirs[] = {
            {0,  0 },
            {1,  0 },
            {-1, 0 },
            {0,  1 },
            {0,  -1},
            {1,  1 },
            {-1, 1 },
            {1,  -1},
            {-1, -1}
        };

        bool resolved = false;
        for (int r = 1; r <= 16 && !resolved; ++r) {
            for (const auto &[x, y] : search_dirs) {
                Vector2 candidate = {
                    position.x + x * static_cast<float>(r),
                    position.y + y * static_cast<float>(r)};

                if (!collides(candidate)) {
                    position = candidate;
                    resolved = true;
                    break;
                }
            }
        }

        set_position(position);
    }
}

auto player::try_pickup_item() -> bool {
    auto [ax, ay] = look_at(get_direction());

    const auto carried_item = get_current_level()->try_lift_object_at(ax, ay);
    if (carried_item == carry_object_type::none) {
        return false;
    }

    set_animation("grab");

    if (const auto sound = load_sound("lift.wav"); IsSoundValid(sound)) {
        PlaySound(sound);
    }

    set_carried_object(carried_item);

    lift_show_timer_ = 0.1f;

    return true;
}

auto player::get_tile_facing() const -> int {
    const auto [x, y] = get_position();
    const auto [dirx, diry] = get_direction_vector(get_direction());

    const auto lx = static_cast<int>(x + 16 + dirx * 24);
    const auto ly = static_cast<int>(y + 16 + diry * 24);

    return get_tile_type(lx, ly);
}

auto player::drop_carried_object() -> bool {
    if (!is_carrying()) {
        return false;
    }

    const auto [x, y] = get_position();

    spawn_thrown_item(get_carried_object(), {x, y - 40.0f}, get_direction());

    set_carried_object(carry_object_type::none);

    return true;
}

auto player::check_jump(const float dt, Vector2 &position) -> bool {
    if (state_ == player_state::jump) {
        return jump_update(dt, position);
    }

    if (!can_jump(position)) {
        return false;
    }

    jump();

    return true;
}

auto player::can_jump(const Vector2 &position) const -> bool {
    if (state_ != player_state::push) {
        return false;
    }

    if (const auto tile = get_tile_facing(); !(tile & tile_type::jump)) {
        return false;
    }

    const auto dir = get_direction();
    const auto x = position.x + jump_frames[static_cast<int>(dir)][7].x;
    const auto y = position.y + jump_frames[static_cast<int>(dir)][7].y;

    if (on_wall({x, y, 31, 31})) {
        return false;
    }

    return true;
}

void player::jump() {
    const auto dir = get_direction();

    set_animation("walk");

    PlaySound(jump_sound_);

    state_ = player_state::jump;
    jump_step_ = 0;
    jump_timer_ = 0;
    jump_origin_ = get_position();
    jump_from_ = jump_origin_;
    jump_to_.x = jump_from_.x + jump_frames[static_cast<int>(dir)][jump_step_].x;
    jump_to_.y = jump_from_.y + jump_frames[static_cast<int>(dir)][jump_step_].y;
}

auto player::jump_update(const float dt, Vector2 &position) -> bool {
    jump_timer_ += dt;

    if (jump_timer_ >= jump_speed) {
        position = jump_to_;

        jump_timer_ = 0;
        jump_step_++;

        if (jump_step_ >= 8) {
            state_ = player_state::idle;

            return false;
        }

        const auto dir = get_direction();

        jump_from_ = get_position();
        jump_to_.x = jump_origin_.x + jump_frames[static_cast<int>(dir)][jump_step_].x;
        jump_to_.y = jump_origin_.y + jump_frames[static_cast<int>(dir)][jump_step_].y;
    }

    position = Vector2Lerp(jump_from_, jump_to_, jump_timer_ / jump_speed);

    return true;
}

bool player::get_carried_destination_override(Vector2 &dest) const {
    if (state_ == player_state::lift) {
        dest = get_position() + get_direction_vector(get_direction()) * 32.0f;

        return true;
    }

    return false;
}
