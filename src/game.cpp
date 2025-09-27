#include "game.hpp"

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include "file_manager.hpp"
#include "level_manager.hpp"
#include "player.hpp"
#include "sound_manager.hpp"
#include "texture_manager.hpp"
#include "thrown_item.hpp"
#include "tileset_manager.hpp"
#include "gui/gui_button_ctrl.hpp"
#include "gui/gui_check_box_ctrl.hpp"
#include "gui/gui_control.hpp"
#include "gui/gui_radio_ctrl.hpp"
#include "gui/gui_window_ctrl.hpp"

#define SIGN_WIDTH 382
#define SIGN_HEIGHT 142

namespace
{
	struct level_info
	{
		level_info(const std::shared_ptr<level> &level, tileset *tileset) : level(level), tileset(tileset)
		{
		}

		std::shared_ptr<level> level;
		tileset *tileset;
	};

	std::unique_ptr<sign> current_sign;
	level_info *current_level_info;
	player *local_player;
	Texture2D state_texture{};
	Texture2D sprites_texture{};
	Font font_24{};
	Font font_18{};
	Font font_pixel_12{};
	std::vector<thrown_item> active_thrown_items{};
	std::vector<leap_effect> active_leaps{};
	std::shared_ptr<gui_control> gui;

	void update_thrown_items(const float dt)
	{
		for (auto &item: active_thrown_items)
		{
			item.update(dt);

			if (!item.is_alive())
			{
				spawn_leaps(item.get_leap_type(), item.get_position());
			}
		}

		std::erase_if(active_thrown_items, [](const thrown_item &item) {
			return !item.is_alive();
		});
	}

	void update_leaps(const float dt)
	{
		for (auto &leap: active_leaps)
		{
			leap.update(dt);
		}

		std::erase_if(active_leaps, [](const leap_effect &item) {
			return !item.is_alive();
		});
	}

	void update()
	{
		const auto dt = GetFrameTime();

		gui->update(dt);

		update_thrown_items(dt);
		update_leaps(dt);

		if (current_sign->is_open())
		{
			current_sign->update();

			return;
		}

		local_player->update(GetFrameTime());
	}

	void draw_hud_key(const Texture &texture, const Font &font, const Vector2 position, const char *key)
	{
		DrawTextureRec(texture, {202, 0, 22, 30}, position, WHITE);

		DrawTextEx(font, key, {position.x + 3, position.y + 4}, 24, 0.0f, BLACK);
		DrawTextEx(font, key, {position.x + 2, position.y + 3}, 24, 0.0f, WHITE);
	}

	void draw_hud_resource(const Rectangle rect, const Vector2 pos, const std::string &text)
	{
		DrawTextureRec(state_texture, rect, pos, WHITE);

		const auto text_str = text.c_str();
		const auto text_size = MeasureTextEx(font_18, text_str, 18, 1);

		const auto tx = pos.x + 8 - text_size.x / 2;
		const auto ty = pos.y + rect.height;

		DrawTextEx(font_18, text_str, {tx + 2, ty + 2}, 18, 1, BLACK);
		DrawTextEx(font_18, text_str, {tx, ty}, 18, 1, WHITE);
	}

	void draw_hud()
	{
		/* Buttons */
		draw_hud_key(state_texture, font_24, {15, 30}, "A");
		draw_hud_key(state_texture, font_24, {80, 30}, "S");
		draw_hud_key(state_texture, font_24, {145, 30}, "D");

		/* Alignment */
		DrawTextureRec(state_texture, {0, 97, 130, 22}, {15, 65}, WHITE);
		DrawTextureRec(state_texture, {0, 119, 100, 10}, {37, 71}, WHITE);

		draw_hud_resource({80, 33, 16, 16}, {274, 30}, "754");
		draw_hud_resource({136, 33, 16, 16}, {274 + 56, 30}, "5");
		draw_hud_resource({184, 33, 16, 16}, {274 + 104, 30}, "0");

		gui->draw();
	}

	void draw_diagnostics()
	{
		const auto str = TextFormat("FPS: %d", GetFPS());

		DrawTextEx(font_pixel_12, str, {6, 6}, 12, 1, BLACK);
		DrawTextEx(font_pixel_12, str, {5, 5}, 12, 1, WHITE);
	}

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

	void draw()
	{
		constexpr auto camera_offset = Vector2(16, 16);
		const auto screen_size = Vector2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
		const auto screen_half_size = screen_size / 2.0f;
		const auto [cx, cy] = screen_half_size - local_player->get_position() - camera_offset;

		rlSetTexture(current_level_info->tileset->get_texture().id);

		rlPushMatrix();
		rlTranslatef(cx, cy, 0);

		if (current_level_info->level)
		{
			current_level_info->level->draw(current_level_info->tileset);
		}

		local_player->draw();

		draw_thrown_items(active_thrown_items);
		draw_leaps(active_leaps);

		rlPopMatrix();
	}

	void init()
	{
		build_file_table("levels");

		constexpr Vector2 pos{512, 512};

		local_player = new player();
		local_player->set_position(pos);

		current_level_info = new level_info(
			load_level("onlinestartlocal.nw"),
			load_tileset("pics1.png"));

		state_texture = load_texture("state.png");

		font_24 = LoadFontEx("levels/fonts/LiberationSans-Bold.ttf", 24, nullptr, 250);
		font_18 = LoadFontEx("levels/fonts/LiberationSans-Regular.ttf", 18, nullptr, 250);
		font_pixel_12 = LoadFontEx("levels/fonts/Kenney Pixel.ttf", 12, nullptr, 0);

		current_sign = std::make_unique<sign>();

		sprites_texture = load_texture("sprites.png");

		const auto gui_window_profile = std::make_shared<gui_control_profile>();
		gui_window_profile->set_texture("guiblue_window.png");
		gui_window_profile->set_font("LiberationSans-Regular.ttf");

		const auto gui_window = std::make_shared<gui_window_ctrl>();
		gui_window->set_profile(gui_window_profile);
		gui_window->set_position({200, 200});
		gui_window->set_size({150, 150});
		gui_window->set_text("GuiWindowCtrl");

		const auto gui_button_profile = std::make_shared<gui_control_profile>();
		gui_button_profile->set_texture("guiblue_button.png");
		gui_button_profile->set_font("LiberationSans-Regular.ttf");

		const auto gui_button = std::make_shared<gui_button_ctrl>();
		gui_button->set_profile(gui_button_profile);
		gui_button->set_position({10, 30});
		gui_button->set_size({110, 25});
		gui_button->set_text("GuiButtonCtrl");
		gui_button->clicked = [] {
			show_sign("Hello World");
		};

		const auto gui_check_box_profile = std::make_shared<gui_control_profile>();
		gui_check_box_profile->set_texture("guiblue_check.png");
		gui_check_box_profile->set_font("LiberationSans-Regular.ttf");

		const auto gui_check_box = std::make_shared<gui_check_box_ctrl>();
		gui_check_box->set_profile(gui_check_box_profile);
		gui_check_box->set_position({10, 130});
		gui_check_box->set_size({90, 25});
		gui_check_box->set_text("GuiCheckBoxCtrl");

		const auto gui_radio_profile = std::make_shared<gui_control_profile>();
		gui_radio_profile->set_texture("guiblue_radio.png");
		gui_radio_profile->set_font("LiberationSans-Regular.ttf");

		const auto gui_radio_1 = std::make_shared<gui_radio_ctrl>();
		gui_radio_1->set_profile(gui_radio_profile);
		gui_radio_1->set_position({10, 160});
		gui_radio_1->set_size({90, 25});
		gui_radio_1->set_text("GuiRadioCtrl[0]");

		const auto gui_radio_2 = std::make_shared<gui_radio_ctrl>();
		gui_radio_2->set_profile(gui_radio_profile);
		gui_radio_2->set_position({10, 190});
		gui_radio_2->set_size({90, 25});
		gui_radio_2->set_text("GuiRadioCtrl[1]");

		const auto gui_radio_3 = std::make_shared<gui_radio_ctrl>();
		gui_radio_3->set_profile(gui_radio_profile);
		gui_radio_3->set_position({10, 220});
		gui_radio_3->set_size({90, 25});
		gui_radio_3->set_text("GuiRadioCtrl[2]");

		const auto gui_text_profile = std::make_shared<gui_control_profile>();
		gui_text_profile->set_texture("guiblue_radio.png");
		gui_text_profile->set_font("LiberationSans-Regular.ttf");
		gui_text_profile->set_text_shadow(true);
		gui_text_profile->set_shadow_offset({2, 2});

		const auto gui_text = std::make_shared<gui_text_ctrl>();
		gui_text->set_profile(gui_text_profile);
		gui_text->set_position({10, 230});
		gui_text->set_size({90, 25});
		gui_text->set_text("GuiTextCtrl");

		gui_window->add_child(gui_button);

		gui = std::make_shared<gui_control>();
		gui->add_child(gui_check_box);
		gui->add_child(gui_radio_1);
		gui->add_child(gui_radio_2);
		gui->add_child(gui_radio_3);
		gui->add_child(gui_text);
		gui->add_child(gui_window);
	}
}

void run_game()
{
	init();

	while (!WindowShouldClose())
	{
		BeginDrawing();

		ClearBackground(BLACK);

		draw();
		draw_hud();

		current_sign->draw(SIGN_WIDTH, SIGN_HEIGHT);

		draw_diagnostics();

		update();

		EndDrawing();
	}
}

auto get_current_level() -> const std::shared_ptr<level> &
{
	return current_level_info->level;
}

void change_level(const std::string &level_name)
{
	const auto level = load_level(level_name);

	if (level == nullptr)
	{
		return;
	}

	current_level_info->level = level;
}

auto on_wall(const Rectangle rect) -> bool
{
	if (current_level_info->level == nullptr)
	{
		return true;
	}

	return current_level_info->level->on_wall(current_level_info->tileset, rect);
}

auto on_wall(const Vector2 pt) -> bool
{
	if (current_level_info->level == nullptr)
	{
		return true;
	}

	return current_level_info->level->on_wall(current_level_info->tileset, pt);
}

auto get_tile_type(const int x, const int y) -> int
{
	if (current_level_info->level == nullptr)
	{
		return tile_type::passable;
	}

	return current_level_info->level->get_tile_type(current_level_info->tileset, x, y);
}

void show_sign(const std::string &str)
{
	current_sign->show(str);
}

void spawn_thrown_item(const carry_object_type type, const Vector2 origin, const direction dir)
{
	if (type == carry_object_type::none)
	{
		return;
	}

	active_thrown_items.emplace_back(type, origin, dir);
}

void spawn_leaps(leap_effect_type type, Vector2 origin)
{
	play_sound("crush.wav");

	active_leaps.emplace_back(type, origin);
}
