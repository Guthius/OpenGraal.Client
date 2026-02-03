#pragma once

#include <raylib.h>

enum class direction {
    up,
    left,
    down,
    right
};

static constexpr auto get_direction_key(const direction direction) -> int {
    switch (direction) {
    case direction::up:    return KEY_UP;
    case direction::left:  return KEY_LEFT;
    case direction::down:  return KEY_DOWN;
    case direction::right: return KEY_RIGHT;
    default:               return KEY_NULL;
    }
}

static constexpr Vector2 get_direction_vector(const direction dir) {
    switch (dir) {
    case direction::up:    return {0, -1};
    case direction::down:  return {0, 1};
    case direction::left:  return {-1, 0};
    case direction::right: return {1, 0};
    default:               return {0, 0};
    }
}

static constexpr direction get_opposite_direction(const direction dir) {
    switch (dir) {
    case direction::up:    return direction::down;
    case direction::left:  return direction::right;
    case direction::down:  return direction::up;
    case direction::right: return direction::left;
    default:               return dir;
    }
}

static constexpr auto get_opposite_direction_key(const direction direction) -> int {
    switch (direction) {
    case direction::up:    return KEY_DOWN;
    case direction::left:  return KEY_RIGHT;
    case direction::down:  return KEY_UP;
    case direction::right: return KEY_LEFT;
    default:               return KEY_NULL;
    }
}
