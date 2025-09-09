#pragma once

typedef enum
{
	DIR_UP,
	DIR_LEFT,
	DIR_DOWN,
	DIR_RIGHT
} Direction;

static constexpr int GetOppositeDirection(const int dir)
{
	switch (dir)
	{
		case DIR_UP:
			return DIR_DOWN;
		case DIR_LEFT:
			return DIR_RIGHT;
		case DIR_DOWN:
			return DIR_UP;
		case DIR_RIGHT:
			return DIR_LEFT;
		default:
			return dir;
	}
}
