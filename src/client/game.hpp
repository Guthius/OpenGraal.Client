#pragma once

#include "constants.hpp"
#include "gui/gui_control.hpp"
#include "leap_effect.hpp"
#include "level.hpp"
#include "network.hpp"

#include <array>

struct network_settings;
struct screenshot_settings;

void run_game(const network_settings *network, const screenshot_settings *screenshot);

class player;

[[nodiscard]] auto get_gui() -> const std::shared_ptr<gui_control> &;
[[nodiscard]] auto get_local_player() -> player *;
[[nodiscard]] auto screen_to_world(const Vector2 &screen) -> Vector2;
[[nodiscard]] auto world_to_screen(const Vector2 &world) -> Vector2;

void refresh_tileset();

[[nodiscard]] auto get_current_level() -> const std::shared_ptr<level> &;
[[nodiscard]] auto get_tile_type(int x, int y) -> int;
[[nodiscard]] auto on_wall(Rectangle rect) -> bool;
[[nodiscard]] auto on_wall(Vector2 pt) -> bool;

void change_level(const std::string &level_name);

void set_gmap_window(int cell_x, int cell_y, const std::array<std::string, 9> &neighbours, const network::gmap_terrain &terrain);

void clear_gmap_window();

void show_sign(const std::string &str);
void spawn_thrown_item(carry_object_type type, Vector2 origin, direction dir);
void spawn_leaps(leap_effect_type type, Vector2 origin);
