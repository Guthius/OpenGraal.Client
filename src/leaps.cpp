#include "leaps.hpp"

#include "texture_manager.hpp"

static constexpr Rectangle sprites[] = {
	{35, 8, 9, 8},
	{44, 0, 16, 16},
	{60, 0, 16, 16},
	{24, 82, 16, 16},
	{40, 82, 16, 16},
	{24, 98, 16, 16},
	{40, 98, 16, 16},
	{56, 82, 16, 14},
	{72, 82, 16, 14},
	{56, 96, 16, 14},
	{72, 96, 16, 14},
};

LeapFrameSet frame_set_leaves = {
	{{1, 0, 4}, {1, 6, 0}, {2, 6, 8}, {2, 9, 17}},
	{{1, -2, 4}, {1, 4, -4}, {2, 8, 10}, {2, 9, 19}},
	{{2, -4, 4}, {1, 2, -8}, {2, 10, 12}, {2, 9, 21}},
	{{1, -5, 4}, {1, 1, -10}, {1, 9, 13}, {1, 9, 22}},
	{{1, -6, 4}, {1, 0, -12}, {1, 10, 14}, {1, 10, 23}},
	{{2, -7, 4}, {1, -1, -14}, {2, 10, 15}, {2, 10, 24}},
	{{2, -8, 4}, {1, -2, -16}, {2, 10, 22}, {2, 10, 25}},
};

LeapFrameSet frame_set_grass = {
	{{0, 0, 8}, {0, 8, 0}, {0, 12, 8}},
	{{0, -2, 8}, {0, 8, -2}, {0, 14, 8}},
	{{0, -4, 8}, {0, 8, -4}, {0, 16, 8}},
	{{0, -6, 8}, {0, 8, -6}, {0, 18, 8}},
};

LeapFrameSet frame_set_stone = {
	{{3, -1, 0}, {3, 0, 9}, {5, 9, -1}, {5, 10, 8}},
	{{6, -3, -2}, {6, -2, 11}, {4, 11, -3}, {4, 12, 10}},
	{{3, -5, -4}, {3, -4, 13}, {5, 13, -5}, {5, 14, 12}},
	{{6, -7, -6}, {6, -6, 15}, {4, 15, -7}, {4, 16, 14}},
};

LeapFrameSet frame_set_wood = {
	{{7, -1, 0}, {7, 0, 9}, {9, 9, -1}, {9, 10, 8}},
	{{10, -3, -2}, {10, -2, 11}, {8, 11, -3}, {8, 12, 10}},
	{{7, -5, -4}, {7, -4, 13}, {9, 13, -5}, {9, 14, 12}},
	{{10, -7, -6}, {10, -6, 15}, {8, 15, -7}, {8, 16, 14}},
};

static constexpr auto frame_duration = 0.05f;

static const LeapFrameSet &get_frame_set(const LeapType type)
{
	switch (type)
	{
		default:
		case LeapType::Leaves:
			return frame_set_leaves;

		case LeapType::Grass:
			return frame_set_grass;

		case LeapType::Stone:
			return frame_set_stone;

		case LeapType::Wood:
			return frame_set_wood;
	}
}

Leap::Leap(const LeapType type, const Vector2 position)
	: texture_(texture_manager::Get("sprites.png")),
	  type_(type), position_(position),
	  frame_set_(get_frame_set(type))
{
}

void Leap::Update(const float dt)
{
	if (!alive_)
	{
		return;
	}

	frame_time_ += dt;
	if (frame_time_ >= frame_duration)
	{
		frame_++;
		if (frame_ >= frame_set_.size())
		{
			alive_ = false;
			return;
		}

		frame_time_ -= frame_duration;
	}
}

void Leap::Draw() const
{
	for (const auto &[sprite, x, y]: frame_set_[frame_])
	{
		const float dx = position_.x + (x * 2);
		const float dy = position_.y + (y * 2);

		DrawTextureRec(texture_, sprites[sprite], {dx, dy}, WHITE);
	}
}
