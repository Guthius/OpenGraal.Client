#pragma once

#include <cstdint>
#include <raylib.h>
#include <vector>

struct leap_frame_part
{
	uint8_t sprite;
	float x;
	float y;
};

using leap_frame = std::vector<leap_frame_part>;
using leap_frame_set = std::vector<leap_frame>;

enum class leap_effect_type
{
	none,
	bush,
	swamp,
	stone,
	sign

	// TODO: ball and water?
};

class leap_effect
{
public:
	leap_effect(leap_effect_type type, Vector2 position);

	[[nodiscard]] auto is_alive() const -> bool { return alive_; }

	void update(float dt);
	void draw() const;

private:
	Texture2D texture_;
	leap_effect_type type_;
	Vector2 position_;
	leap_frame_set frame_set_;
	float frame_time_{0.0f};
	int frame_{0};
	bool alive_{true};
};
