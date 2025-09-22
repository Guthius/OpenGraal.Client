#pragma once

#include "Actor.h"
#include "Leaps.h"

class ThrownItem final
{
public:
	ThrownItem(Actor::CarriedItem type, Vector2 origin, Direction dir);

	auto GetType() const -> Actor::CarriedItem { return type_; }
	auto GetLeapType() const -> LeapType { return leap_type_; }
	auto GetPosition() const -> const Vector2 & { return position_; }
	auto IsAlive() const -> bool { return alive_; }
	void Update(float dt);
	void Draw() const;

private:
	void DrawShadow() const;

	Actor::CarriedItem type_{Actor::CarriedItem::None};
	Vector2 start_{0, 0};
	Vector2 end_{0, 0};
	Vector2 position_{0, 0};
	float time_elapsed_{0.0f};
	float duration_{0.35f};
	Direction dir_{Direction::DIR_DOWN};
	bool alive_{true};
	mutable Texture2D sprites_{};
	LeapType leap_type_;
};
