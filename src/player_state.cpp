#include "input.hpp"
#include "player.hpp"
#include "tileset.hpp"

#include <raylib.h>
#include <raymath.h>

namespace {
    auto is_trying_to_move() -> bool {
        return is_action_down(input_action::up) ||
               is_action_down(input_action::down) ||
               is_action_down(input_action::left) ||
               is_action_down(input_action::right);
    }

    auto is_trying_to_attack() -> bool {
        return is_action_pressed(input_action::attack);
    }

    auto is_trying_to_grab() -> bool {
        return is_action_pressed(input_action::grab);
    }

    auto get_animation_for_state(const player_state state) -> std::string {
        switch (state) {
        default:
        case player_state::idle:       return "idle";
        case player_state::walk:       return "walk";
        case player_state::grab:       return "grab";
        case player_state::push:       return "push";
        case player_state::pull:       return "pull";
        case player_state::swim:       return "swim";
        case player_state::sit:        return "sit";
        case player_state::jump:       return "jump"; // TODO: ?? jump anim ??
        case player_state::attack:     return "sword";
        case player_state::lift:       return "grab";
        case player_state::carrystill: return "carrystill";
        case player_state::carry:      return "carry";
        }
    }
}

void player::state_enter(const player_state state) {
    set_animation(get_animation_for_state(state));

    if (state != player_state::carry &&
        state != player_state::carrystill &&
        state != player_state::lift) {
        if (is_carrying()) {
            drop_carried_object();
        }
    }

    switch (state) {
    case player_state::walk:
    case player_state::carry:
        push_timer_ = 0;
        break;

    case player_state::swim:
        break;

    case player_state::lift:
        lift_show_timer_ = 0.1f;
        break;

    default:
        break;
    }
}

auto player::state_update(const player_state state, const float dt) -> player_state {
    auto set_direction_if_needed = [&](Vector2 dir) -> void {
        dir = Vector2Normalize(dir);
        if (dir.y == -1.0f)
            set_direction(direction::up);
        if (dir.y == 1.0f)
            set_direction(direction::down);
        if (dir.x == -1.0f)
            set_direction(direction::left);
        if (dir.x == 1.0f)
            set_direction(direction::right);
    };

    switch (state) {
    case player_state::idle:
        {
            if (is_trying_to_attack())
                return attack();
            if (is_trying_to_grab() && is_facing_wall())
                return try_pickup_item() ? player_state::lift : player_state::grab;
            if (is_trying_to_move())
                return player_state::walk;
        }
        return state;

    case player_state::walk:
        {
            if (is_trying_to_attack())
                return attack();
            if (get_terrain_tile_type() == tile_type::chair)
                return player_state::sit;
            if (get_terrain_tile_type() == tile_type::water)
                return player_state::swim;
            if (is_trying_to_grab() && is_facing_wall())
                return try_pickup_item() ? player_state::lift : player_state::grab;
            if (!is_trying_to_move())
                return player_state::idle;

            const auto input_direction = get_input_direction_vector();
            const auto velocity = Vector2Normalize(input_direction) * walk_speed_;

            set_velocity(velocity);
            set_direction_if_needed(input_direction);

            if (is_facing_wall()) {
                push_timer_ += dt;
                if (push_timer_ >= .75f) {
                    return player_state::push;
                }
            }
        }
        return state;

    case player_state::grab:
        {
            if (!is_action_down(input_action::grab)) {
                return player_state::walk;
            }

            if (IsKeyDown(get_opposite_direction_key(get_direction()))) {
                return player_state::pull;
            }
        }
        return state;

    case player_state::push:
        {
            if (!IsKeyDown(get_direction_key(get_direction()))) {
                return player_state::idle;
            }
        }
        return state;

    case player_state::pull:
        {
            if (!is_action_down(input_action::grab)) {
                return player_state::walk;
            }

            if (!IsKeyDown(get_opposite_direction_key(get_direction()))) {
                return player_state::grab;
            }
        }
        return state;

    case player_state::swim:
    case player_state::sit:
        {
            if (state == player_state::sit) {
                if (is_trying_to_grab() && is_facing_wall()) {
                    return player_state::grab;
                }
            }

            const auto input_direction = get_input_direction_vector();
            const auto velocity = Vector2Normalize(input_direction) * walk_speed_;

            set_velocity(velocity);
            set_direction_if_needed(input_direction);

            if (get_terrain_tile_type() != tile_type::chair &&
                get_terrain_tile_type() != tile_type::water) {
                return player_state::walk;
            }

            if (check_for_sign_at(get_position())) {
                return state;
            }
        }
        return state;

    case player_state::jump:
        return state;

    case player_state::attack:
        {
            if (get_animation_state().ended) {
                return player_state::idle;
            }
        }
        return state;

    case player_state::lift:
        {
            lift_show_timer_ -= dt;

            if (lift_show_timer_ <= 0.0f) {
                return player_state::carrystill;
            }
        }
        return state;

    case player_state::carrystill:
        {
            if (is_action_pressed(input_action::grab) ||
                is_action_pressed(input_action::attack)) {
                drop_carried_object();

                return player_state::idle;
            }

            if (is_trying_to_move())
                return player_state::carry;
        }
        return state;

    case player_state::carry:
        {
            if (is_action_pressed(input_action::grab) ||
                is_action_pressed(input_action::attack)) {
                drop_carried_object();

                return player_state::idle;
            }

            if (get_terrain_tile_type() == tile_type::chair)
                return player_state::sit;
            if (get_terrain_tile_type() == tile_type::water)
                return player_state::swim;

            if (!is_trying_to_move())
                return player_state::carrystill;

            const auto input_direction = get_input_direction_vector();
            const auto velocity = Vector2Normalize(input_direction) * walk_speed_;

            set_velocity(velocity);
            set_direction_if_needed(input_direction);

            if (check_for_sign_at(get_position())) {
                return player_state::carrystill;
            }
        }
        return state;

    default: break;
    }

    return state;
}

void player::state_exit(const player_state state) {
    if (state == player_state::walk || state == player_state::carry) {
        push_timer_ = 0.0f;
    }

    set_velocity({0, 0});
}

auto player::attack() -> player_state {
    if (drop_carried_object()) {
        return player_state::walk;
    }

    try_destroy_object_facing(get_position());

    return player_state::attack;
}

void player::update(const float dt) {
    const auto new_state = state_update(state_, dt);
    if (new_state != state_) {
        state_exit(state_);

        state_ = new_state;

        state_enter(state_);
    }

    if (move(dt)) {
        push_timer_ = 0;

        check_for_sign_at(get_position());
    }

    actor::update(dt);
}
