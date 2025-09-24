#pragma once

#include <vector>

#include "actor.hpp"
#include "leap_effect.hpp"
#include "level.hpp"
#include "player.hpp"
#include "sign.hpp"
#include "thrown_item.hpp"
#include "tileset.hpp"

class game
{
	struct level_info
	{
		level_info(level *level, tileset *tileset, const int x, const int y)
			: level(level), tileset(tileset), x(x), y(y)
		{
		}

		level *level;
		tileset *tileset;
		int x, y;
	};

public:
	game();

	void run();

	[[nodiscard]] auto get_current_level() const -> level * { return level_info_->level; }
	[[nodiscard]] auto get_tile_type(int x, int y) const -> int;
	[[nodiscard]] auto on_wall(Rectangle rect) const -> bool;
	[[nodiscard]] auto on_wall(Vector2 pt) const -> bool;

	void change_level(const std::string &level_name) const;
	void show_sign(const std::string &str) const;
	void spawn_thrown_item(carry_object_type type, Vector2 origin, direction dir);
	void spawn_leaps(leap_effect_type type, Vector2 origin);

private:
	void update();
	void update_thrown_items(float dt);
	void update_leaps(float dt);

	void draw() const;
	void draw_hud() const;
	void draw_hud_resource(Rectangle rect, Vector2 pos, const std::string &text) const;
	void draw_diagnostics() const;

	sign *sign_;
	level_info *level_info_;
	player *player_;
	Texture2D state_{};
	Font font20_{};
	Font font14_{};
	Font font_pixel_{};
	std::vector<thrown_item> thrown_items_{};
	std::vector<leap_effect> leaps_{};
	Texture2D sprites_{};
};
