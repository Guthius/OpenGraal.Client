#include "Leaps.h"

#include "TextureManager.h"

static constexpr Rectangle sprites[] = {
	/* 0 */ {0, 0, 12, 6},
	/* 1 */ {12, 0, 3, 9},
	/* 2 */ {15, 0, 4, 10},
	/* 3 */ {19, 0, 8, 10},
	/* 4 */ {27, 0, 4, 10},
	/* 5 */ {31, 0, 14, 14},
	/* 6 */ {45, 0, 14, 12},
	/* 7 */ {59, 0, 5, 11},
	/* 8 */ {0, 6, 6, 12},
	/* 9 */ {7, 10, 12, 14},
	/* 10 */ {19, 14, 14, 12},
	/* 11 */ {33, 14, 14, 14},
	/* 12 */ {47, 12, 17, 6},
	/* 13 */ {0, 18, 7, 12},
	/* 14 */ {7, 24, 12, 14},
	/* 15 */ {19, 26, 14, 14},
	/* 16 */ {33, 28, 12, 14},
	/* 17 */ {47, 18, 14, 14},
	/* 18 */ {0, 30, 5, 12},
	/* 19 */ {5, 38, 7, 10},
	/* 20 */ {28, 42, 17, 6},
	/* 21 */ {56, 32, 8, 14},
	/* 22 */ {0, 448, 16, 8},
	/* 23 */ {0, 456, 16, 8},

	/* 24 */ {35, 8, 9, 8},

	/* 25 */ {44, 0, 16, 16},
	/* 26 */ {60, 0, 16, 16},

	/* 27 */ {20, 40, 8, 8},
	/* 28 */ {0, 112, 12, 12},
	/* 29 */ {12, 112, 14, 14},
	/* 30 */ {26, 112, 16, 16},
	/* 31 */ {0, 128, 16, 16},
	/* 32 */ {16, 128, 16, 16},
	/* 33 */ {32, 128, 16, 15},
	/* 34 */ {48, 112, 8, 8},
	/* 35 */ {56, 112, 8, 8},
	/* 36 */ {48, 120, 8, 8},
	/* 37 */ {56, 120, 8, 8},
	/* 38 */ {48, 128, 8, 8},
	/* 39 */ {56, 128, 8, 8},
	/* 40 */ {48, 136, 8, 8},
	/* 41 */ {56, 136, 8, 8},
	/* 42 */ {12, 38, 8, 4},
	/* 43 */ {0, 144, 6, 4},
	/* 44 */ {6, 144, 6, 4},
	/* 45 */ {0, 148, 6, 4},
	/* 46 */ {6, 148, 6, 4}
};

LeapFrameSet frame_set = {
	{{25, 0, 4}, {25, 6, 0}, {26, 6, 8}, {26, 9, 17}},
	{{25, -2, 4}, {25, 4, -4}, {26, 8, 10}, {26, 9, 19}},
	{{26, -4, 4}, {25, 2, -8}, {26, 10, 12}, {26, 9, 21}},
	{{25, -5, 4}, {25, 1, -10}, {25, 9, 13}, {25, 9, 22}},
	{{25, -6, 4}, {25, 0, -12}, {25, 10, 14}, {25, 10, 23}},
	{{26, -7, 4}, {25, -1, -14}, {26, 10, 15}, {26, 10, 24}},
	{{26, -8, 4}, {25, -2, -16}, {26, 10, 22}, {26, 10, 25}},
};

LeapFrameSet frame_set_2 = {
	{{24, 0, 8}, {24, 8, 0}, {24, 12, 8}},
	{{24, -2, 8}, {24, 8, -2}, {24, 14, 8}},
	{{24, -4, 8}, {24, 8, -4}, {24, 16, 8}},
	{{24, -6, 8}, {24, 8, -6}, {24, 18, 8}},
};

static constexpr auto frame_duration = 0.05f;

static const LeapFrameSet &get_frame_set(const LeapType type)
{
	switch (type)
	{
		default:
		case LeapType::Leaves:
			return frame_set;

		case LeapType::Grass:
			return frame_set_2;
	}
}

Leap::Leap(const LeapType type, const Vector2 position)
	: texture_(TextureManager::Get("sprites.png")),
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
