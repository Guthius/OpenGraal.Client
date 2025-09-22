#pragma once

#include "animation.hpp"
#include "animation_manager.hpp"
#include "constants.hpp"

#include <string>
#include <raylib.h>

class actor
{
public:
	enum class CarriedItem
	{
		None,
		Bush,
		Sign,
		Vase,
		Stone,
		BlackStone
	};

	actor()
	{
		SetAnimation("idle");
	}

	virtual ~actor() = default;

	virtual void Update(float dt);
	virtual void Draw() const;

	[[nodiscard]] auto GetPosition() const -> Vector2 { return position_; }
	[[nodiscard]] auto GetDirection() const -> Direction { return dir_; }
	[[nodiscard]] auto GetAnimation() const -> const std::string & { return animation_name_; }
	[[nodiscard]] auto GetAnimationState() const -> const AnimationState & { return animation_state_; }
	[[nodiscard]] auto GetCarriedItem() const -> CarriedItem { return carried_object_; }
	[[nodiscard]] auto IsCarrying() const -> bool { return carried_object_ != CarriedItem::None; }

	void SetPosition(const Vector2 &position) { position_ = position; }
	void SetDirection(const Direction dir) { dir_ = dir; }
	void SetAnimation(const std::string &name);
	void SetCarriedItem(const CarriedItem item) { carried_object_ = item; }

protected:
	virtual bool GetCarriedDestinationOverride(Vector2 &dest) const { return false; }

	Vector2 position_{0, 0};
	Direction dir_ = Direction::DIR_UP;
	AnimationState animation_state_{};
	std::string animation_name_{};
	animation *animation_ = nullptr;
	Texture2D sprites_{};
	CarriedItem carried_object_ = CarriedItem::None;
};
