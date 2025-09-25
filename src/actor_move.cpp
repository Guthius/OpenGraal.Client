#include <cmath>
#include <raylib.h>
#include <raymath.h>

#include "actor.hpp"
#include "game.hpp"

namespace
{
	auto try_move(const game *game, Vector2 &position, const Vector2 &velocity) -> bool
	{
		auto collides = [&](const float nx, const float ny) -> bool {
			return game->on_wall(Rectangle{nx, ny, 31.0f, 31.0f});
		};

		auto check_x = [&](float &x, const float delta) {
			const float step_sign = delta > 0.0f ? 1.0f : -1.0f;
			auto remaining = std::fabs(delta);
			while (remaining > 0.0f)
			{
				const float step = std::min(1.0f, remaining);

				if (collides(x + step_sign * step, position.y))
				{
					break;
				}

				x += step_sign * step;

				remaining -= step;
			}
		};

		auto check_y = [&](float &y, const float delta) {
			const float step_sign = delta > 0.0f ? 1.0f : -1.0f;
			auto remaining = std::fabs(delta);
			while (remaining > 0.0f)
			{
				const float step = std::min(1.0f, remaining);

				if (collides(position.x, y + step_sign * step))
				{
					break;
				}

				y += step_sign * step;

				remaining -= step;
			}
		};

		const auto start_x = position.x;
		const auto start_y = position.y;

		if (velocity.x != 0.0f) check_x(position.x, velocity.x);
		if (velocity.y != 0.0f) check_y(position.y, velocity.y);

		return position.x != start_x || position.y != start_y;
	}
}

auto actor::move(const float dt) -> bool
{
	if (velocity_.x == 0 && velocity_.y == 0)
	{
		return false;
	}

	if (try_move(game_, position_, velocity_ * dt) || slide(dt))
	{
		update_terrain();

		return true;
	}

	return false;
}

bool actor::slide(const float dt)
{
	auto [tile1, tile2] = get_facing_walls(dir_);
	if (tile1 && tile2)
	{
		return false;
	}

	direction slide_dir;
	if (tile1)
	{
		switch (dir_)
		{
			case direction::up:
				slide_dir = direction::right;
				break;
			case direction::left:
				slide_dir = direction::down;
				break;
			case direction::down:
				slide_dir = direction::right;
				break;
			case direction::right:
				slide_dir = direction::down;
				break;
			default:
				return false;
		}
	}
	else
	{
		switch (dir_)
		{
			case direction::up:
				slide_dir = direction::left;
				break;
			case direction::left:
				slide_dir = direction::up;
				break;
			case direction::down:
				slide_dir = direction::left;
				break;
			case direction::right:
				slide_dir = direction::up;
				break;
			default:
				return false;
		}
	}

	if (game_->on_wall(look_at(dir_)))
	{
		return false;
	}

	const auto slide_speed = Vector2DotProduct(velocity_, get_direction_vector(dir_)) * 0.35f * dt;

	return try_move(game_, position_, get_direction_vector(slide_dir) * slide_speed);
}

auto actor::get_facing_walls(const direction dir) const -> std::tuple<bool, bool>
{
	auto [x, y] = get_position();

	Vector2 v1, v2;
	switch (dir)
	{
		case direction::up:
			v1 = {x, y - 1};
			v2 = {x + 31, y - 1};
			break;

		case direction::left:
			v1 = {x - 1, y};
			v2 = {x - 1, y + 31};
			break;

		case direction::down:
			v1 = {x, y + 32};
			v2 = {x + 31, y + 32};
			break;

		case direction::right:
			v1 = {x + 32, y};
			v2 = {x + 32, y + 31};
			break;

		default:
			return {};
	}

	return {
		game_->on_wall(v1),
		game_->on_wall(v2)
	};
}

auto actor::look_at(const direction dir) const -> Vector2
{
	return position_ + Vector2(16, 16) + get_direction_vector(dir) * 32;
}

void actor::update_terrain()
{
	const auto check_x = static_cast<int>(position_.x + 16);
	const auto check_y = static_cast<int>(position_.y + 24);

	terrain_tile_type_ = game_->get_tile_type(check_x, check_y);
	if (terrain_tile_type_ & tile_type::swamp)
	{
		set_terrain(terrain_effect_type::swamp);
	}
	else if (terrain_tile_type_ & tile_type::near_water)
	{
		set_terrain(terrain_effect_type::near_water);
	}
	else
	{
		set_terrain(terrain_effect_type::none);
	}
}
