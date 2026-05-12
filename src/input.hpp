#pragma once

#include <raylib.h>

enum class input_action {
    up,
    left,
    down,
    right,
    shoot,
    attack,
    grab,
    map,
    chat,
    inventory
};

auto is_action_down(input_action action) -> bool;
auto is_action_up(input_action action) -> bool;
auto is_action_pressed(input_action action) -> bool;
auto is_action_released(input_action action) -> bool;
auto get_input_direction_vector() -> Vector2;
