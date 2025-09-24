#pragma once

#include <filesystem>
#include <map>
#include <raylib.h>
#include <string>
#include <vector>

#include "constants.hpp"

class animation;

struct animation_state
{
	size_t current_frame;
	float current_frame_duration;
	std::string body{"body.png"};
	std::string head{"head0.png"};
	std::string sword{"sword1.png"};
	std::string shield{"shield1.png"};
	std::string attr1{"hat0.png"};
	bool ended;

	void reset(size_t frame, const animation *animation);
};

enum class sprite_source
{
	file,
	sprites,
	shield,
	sword,
	head,
	body,
	attr1
};

class animation
{
	struct sprite
	{
		int id;
		sprite_source source;
		std::string texture;
		Rectangle texture_rect;
	};

	struct sprite_ref
	{
		sprite *sprite;
		Vector2 position;
	};

	struct frame
	{
		std::vector<sprite_ref> sprites[4]{};
		float duration;
		std::string play_sound;
		Vector2 play_sound_at;
	};

public:
	animation() = default;

private:
	void parse_sprite(const std::vector<std::string> &tokens);
	void parse_ani(std::ifstream &stream);
	void parse_sprites(std::string &line, std::vector<sprite_ref> &frame);

public:
	void load_file(const std::filesystem::path &path);
	void update(float dt, animation_state &state) const;
	void draw(float x, float y, direction direction, const animation_state &state) const;
	void play_sound(size_t frame) const;

	[[nodiscard]]
	auto frame_count() const -> size_t { return frames_.size(); }

	[[nodiscard]]
	auto frame_duration(const size_t frame) const -> float { return frames_[frame].duration; }

private:
	void draw_sprites(const animation_state &state, const std::vector<sprite_ref> &sprite_refs) const;

	[[nodiscard]] auto get_texture_filename(const animation_state &state, const sprite_ref &sprite_ref) const -> std::string;

	std::string set_back_to_;
	std::string default_attr1_{"hat0.png"};
	std::string default_head_{"head19.png"};
	std::string default_body_{"body.png"};
	bool single_direction_ = false;
	bool continuous_ = false;
	std::map<int, sprite> sprites_{};
	std::vector<frame> frames_{};
};
