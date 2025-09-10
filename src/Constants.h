#pragma once

enum class Direction
{
	DIR_UP,
	DIR_LEFT,
	DIR_DOWN,
	DIR_RIGHT
};

static constexpr Direction GetOppositeDirection(const Direction dir)
{
	switch (dir)
	{
		case Direction::DIR_UP:
			return Direction::DIR_DOWN;
		case Direction::DIR_LEFT:
			return Direction::DIR_RIGHT;
		case Direction::DIR_DOWN:
			return Direction::DIR_UP;
		case Direction::DIR_RIGHT:
			return Direction::DIR_LEFT;
		default:
			return dir;
	}
}
