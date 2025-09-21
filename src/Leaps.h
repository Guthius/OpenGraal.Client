#pragma once

#include "Actor.h"

struct LeapFramePart
{
	uint8_t sprite;
	float x;
	float y;
};

using LeapFrame = std::vector<LeapFramePart>;
using LeapFrameSet = std::vector<LeapFrame>;

enum class LeapType
{
	None,
	Leaves,
	Grass,
};

class Leap
{
public:
	Leap(LeapType type, Vector2 position);

	auto IsAlive() const -> bool { return alive_; }
	void Update(float dt);
	void Draw() const;

private:
	Texture2D texture_;
	LeapType type_;
	Vector2 position_;
	LeapFrameSet frame_set_;
	float frame_time_{0.0f};
	int frame_{0};
	bool alive_{true};
};
