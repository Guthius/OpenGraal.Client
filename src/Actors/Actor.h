#pragma once

#include "../Animation.h"
#include "../AnimationManager.h"
#include "../Constants.h"

#include <string>
#include <raylib.h>

class Actor
{
public:
	Actor()
	{
		SetAnimation("idle");
	}

public:
	virtual void Update(float dt);

	void Draw() const;

public:
	[[nodiscard]] Vector2 GetPosition() const { return _position; }
	[[nodiscard]] Direction GetDirection() const { return _dir; }
	[[nodiscard]] const std::string &GetAnimation() const { return _animationName; }

	void SetPosition(Vector2 &position) { _position = position; }
	void SetDirection(Direction dir) { _dir = dir; }
	void SetAnimation(const std::string &name);

private:
	Vector2 _position{0, 0};
	Direction _dir = Direction::Up;
	AnimationState _animationState{};
	std::string _animationName{};
	Animation *_animation = nullptr;
};