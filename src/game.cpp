#include "game.hpp"

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include "file_manager.hpp"
#include "level_manager.hpp"
#include "sound_manager.hpp"
#include "texture_manager.hpp"
#include "tileset_manager.hpp"

#define SIGN_WIDTH 382
#define SIGN_HEIGHT 142

namespace
{
	void draw_thrown_items(const std::vector<thrown_item> &thrown_items)
	{
		for (const auto &thrown_item: thrown_items)
		{
			thrown_item.draw();
		}
	}

	void draw_leaps(const std::vector<leap_effect> &leaps)
	{
		for (const auto &leap: leaps)
		{
			leap.draw();
		}
	}
}

game::game()
{
	build_file_table("levels");

	constexpr Vector2 pos{512, 512};

	player_ = new player(this);
	player_->set_position(pos);

	level_info_ = new level_info(
		load_level("onlinestartlocal.nw"),
		load_tileset("pics1.png"),
		0, 0);

	state_ = load_texture("state.png");
	font20_ = LoadFontEx("Fonts/LiberationSans-Bold.ttf", 24, nullptr, 250);
	font14_ = LoadFontEx("Fonts/LiberationSans-Regular.ttf", 18, nullptr, 250);

	font_pixel_ = LoadFontEx("Fonts/Kenney Pixel.ttf", 12, nullptr, 0);

	sign_ = new sign();

	sprites_ = load_texture("sprites.png");
}

void game::run()
{
	while (!WindowShouldClose())
	{
		BeginDrawing();

		ClearBackground(BLACK);

		draw();
		draw_hud();

		sign_->draw(SIGN_WIDTH, SIGN_HEIGHT);

		draw_diagnostics();

		update();

		EndDrawing();
	}
}

void game::change_level(const std::string &level_name) const
{
	const auto level = load_level(level_name);

	if (level == nullptr)
	{
		return;
	}

	level_info_->level = level;
}

auto game::on_wall(const Rectangle rect) const -> bool
{
	if (level_info_->level == nullptr)
	{
		return true;
	}

	return level_info_->level->on_wall(level_info_->tileset, rect);
}

auto game::on_wall(const Vector2 pt) const -> bool
{
	if (level_info_->level == nullptr)
	{
		return true;
	}

	return level_info_->level->on_wall(level_info_->tileset, pt);
}

auto game::get_tile_type(const int x, const int y) const -> int
{
	if (level_info_->level == nullptr)
	{
		return tile_type::passable;
	}

	return level_info_->level->get_tile_type(level_info_->tileset, x, y);
}

void game::show_sign(const std::string &str) const
{
	sign_->show(str);
}

void game::update()
{
	const auto dt = GetFrameTime();

	update_thrown_items(dt);
	update_leaps(dt);

	if (sign_->is_open())
	{
		sign_->update();

		return;
	}

	player_->update(GetFrameTime());
}

void game::update_thrown_items(const float dt)
{
	for (auto &item: thrown_items_)
	{
		item.update(dt);

		if (!item.is_alive())
		{
			spawn_leaps(item.get_leap_type(), item.get_position());
		}
	}

	std::erase_if(thrown_items_, [](const thrown_item &item) {
		return !item.is_alive();
	});
}

void game::update_leaps(const float dt)
{
	for (auto &leap: leaps_)
	{
		leap.update(dt);
	}

	std::erase_if(leaps_, [](const leap_effect &item) {
		return !item.is_alive();
	});
}

void game::draw() const
{
	constexpr auto camera_offset = Vector2(16, 16);
	const auto screen_size = Vector2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
	const auto screen_half_size = screen_size / 2.0f;
	const auto [cx, cy] = screen_half_size - player_->get_position() - camera_offset;

	rlSetTexture(level_info_->tileset->get_texture().id);

	rlPushMatrix();
	rlTranslatef(cx, cy, 0);

	level_info_->level->draw(level_info_->tileset);

	rlSetTexture(0);

	player_->draw();

	draw_thrown_items(thrown_items_);
	draw_leaps(leaps_);

	rlPopMatrix();
}

void draw_hud_key(const Texture &texture, const Font &font, const Vector2 position, const char *key)
{
	DrawTextureRec(texture, {202, 0, 22, 30}, position, WHITE);

	DrawTextEx(font, key, {position.x + 3, position.y + 4}, 24, 0.0f, BLACK);
	DrawTextEx(font, key, {position.x + 2, position.y + 3}, 24, 0.0f, WHITE);
}

void game::spawn_thrown_item(const carry_object_type type, const Vector2 origin, const direction dir)
{
	if (type == carry_object_type::none)
	{
		return;
	}

	thrown_items_.emplace_back(type, origin, dir);
}

void game::spawn_leaps(leap_effect_type type, Vector2 origin)
{
	play_sound("crush.wav");

	leaps_.emplace_back(type, origin);
}

void game::draw_hud() const
{
	/* Buttons */
	draw_hud_key(state_, font20_, {15, 30}, "A");
	draw_hud_key(state_, font20_, {80, 30}, "S");
	draw_hud_key(state_, font20_, {145, 30}, "D");

	/* Alignment */
	DrawTextureRec(state_, {0, 97, 130, 22}, {15, 65}, WHITE);
	DrawTextureRec(state_, {0, 119, 100, 10}, {37, 71}, WHITE);

	draw_hud_resource({80, 33, 16, 16}, {274, 30}, "754");
	draw_hud_resource({136, 33, 16, 16}, {274 + 56, 30}, "5");
	draw_hud_resource({184, 33, 16, 16}, {274 + 104, 30}, "0");
}

void game::draw_hud_resource(const Rectangle rect, const Vector2 pos, const std::string &text) const
{
	DrawTextureRec(state_, rect, pos, WHITE);

	const auto textStr = text.c_str();
	const auto textSize = MeasureTextEx(font14_, textStr, font14_.baseSize, 1);

	const auto tx = pos.x + 8 - textSize.x / 2;
	const auto ty = pos.y + rect.height;

	DrawTextEx(font14_, textStr, {tx + 2, ty + 2}, font14_.baseSize, 1, BLACK);
	DrawTextEx(font14_, textStr, {tx, ty}, font14_.baseSize, 1, WHITE);
}

void game::draw_diagnostics() const
{
	const auto str = TextFormat("FPS: %d", GetFPS());

	DrawTextEx(font_pixel_, str, {6, 6}, 12, 1, BLACK);
	DrawTextEx(font_pixel_, str, {5, 5}, 12, 1, WHITE);
}
