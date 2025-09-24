#pragma once

enum class direction
{
	up,
	left,
	down,
	right
};

static constexpr Vector2 get_direction_vector(const direction dir)
{
	switch (dir)
	{
		case direction::up: return {0, -1};
		case direction::down: return {0, 1};
		case direction::left: return {-1, 0};
		case direction::right: return {1, 0};
		default: return {0, 0};
	}
}

static constexpr direction get_opposite_direction(const direction dir)
{
	switch (dir)
	{
		case direction::up: return direction::down;
		case direction::left: return direction::right;
		case direction::down: return direction::up;
		case direction::right: return direction::left;
		default: return dir;
	}
}
