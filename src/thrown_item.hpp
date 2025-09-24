#pragma once

#include "actor.hpp"
#include "leap_effect.hpp"

class thrown_item final
{
public:
	thrown_item(carry_object_type type, Vector2 origin, direction dir);

	[[nodiscard]] auto get_type() const -> carry_object_type { return type_; }
	[[nodiscard]] auto get_leap_type() const -> leap_effect_type { return leap_type_; }
	[[nodiscard]] auto get_position() const -> const Vector2 & { return position_; }
	[[nodiscard]] auto is_alive() const -> bool { return alive_; }

	void update(float dt);
	void draw() const;

private:
	void draw_shadow() const;

	carry_object_type type_{carry_object_type::none};
	Vector2 start_{0, 0};
	Vector2 end_{0, 0};
	Vector2 position_{0, 0};
	float time_elapsed_{0.0f};
	float duration_{0.35f};
	direction dir_;
	bool alive_{true};
	Texture2D texture_{};
	Rectangle texture_rect_;
	leap_effect_type leap_type_;
};
