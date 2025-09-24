#pragma once

#include <raylib.h>
#include <string>

#include "animation.hpp"
#include "animation_manager.hpp"
#include "carry_object.hpp"
#include "constants.hpp"

class actor
{
public:
	actor();

	virtual ~actor() = default;

	virtual void update(float dt);
	virtual void draw() const;

	[[nodiscard]] auto get_position() const -> Vector2 { return position_; }
	[[nodiscard]] auto get_direction() const -> direction { return dir_; }
	[[nodiscard]] auto get_animation() const -> const std::string & { return animation_name_; }
	[[nodiscard]] auto get_animation_state() const -> const animation_state & { return animation_state_; }
	[[nodiscard]] auto get_carried_object() const -> carry_object_type { return carried_object_; }
	[[nodiscard]] auto is_carrying() const -> bool { return carried_object_ != carry_object_type::none; }

	void set_position(const Vector2 &position) { position_ = position; }
	void set_direction(const direction dir) { dir_ = dir; }
	void set_animation(const std::string &name);
	void set_carried_object(const carry_object_type item) { carried_object_ = item; }

protected:
	virtual auto get_carried_destination_override(Vector2 &dest) const -> bool { return false; }

	Texture2D sprites_;
	Vector2 position_{};
	direction dir_ = direction::up;
	animation_state animation_state_{};
	std::string animation_name_{};
	animation *animation_ = nullptr;
	carry_object_type carried_object_ = carry_object_type::none;
};
