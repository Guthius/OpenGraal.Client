#pragma once

#include "Animation.h"
#include "AnimationManager.h"
#include "Constants.h"

#include <string>
#include <raylib.h>

class Actor
{
public:
	Actor()
	{
		SetAnimation("idle");
	}

	virtual ~Actor() = default;

	virtual void Update(float dt);
	virtual void Draw() const;

	[[nodiscard]] auto GetPosition() const -> Vector2 { return position_; }
	[[nodiscard]] auto GetDirection() const -> int { return dir_; }
	[[nodiscard]] auto GetAnimation() const -> const std::string & { return animation_name_; }
	[[nodiscard]] auto GetAnimationState() const -> const AnimationState & { return animation_state_; }

	void SetPosition(const Vector2 &position) { position_ = position; }
	void SetDirection(const int dir) { dir_ = dir; }
	void SetAnimation(const std::string &name);

private:
	Vector2 position_{0, 0};
	int dir_ = DIR_UP;
	AnimationState animation_state_{};
	std::string animation_name_{};
	Animation *animation_ = nullptr;
};
