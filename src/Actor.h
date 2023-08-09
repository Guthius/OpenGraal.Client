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

public:
	virtual void Update(float dt);

	virtual void Draw() const;

public:
	[[nodiscard]] Vector2 GetPosition() const { return _position; }
	[[nodiscard]] int GetDirection() const { return _dir; }
	[[nodiscard]] const std::string &GetAnimation() const { return _animationName; }

	void SetPosition(Vector2 &position) { _position = position; }
	void SetDirection(int dir) { _dir = dir; }
	void SetAnimation(const std::string &name);

private:
	Vector2 _position{0, 0};
	int _dir = DIR_UP;
	AnimationState _animationState{};
	std::string _animationName{};
	Animation *_animation = nullptr;
};