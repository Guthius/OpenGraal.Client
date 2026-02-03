#pragma once

#include "actor.hpp"
#include "leap_effect.hpp"
#include "level.hpp"

void run_game();

[[nodiscard]] auto get_current_level() -> const std::shared_ptr<level> &;
[[nodiscard]] auto get_tile_type(int x, int y) -> int;
[[nodiscard]] auto on_wall(Rectangle rect) -> bool;
[[nodiscard]] auto on_wall(Vector2 pt) -> bool;

void change_level(const std::string &level_name);
void show_sign(const std::string &str);
void spawn_thrown_item(carry_object_type type, Vector2 origin, direction dir);
void spawn_leaps(leap_effect_type type, Vector2 origin);
