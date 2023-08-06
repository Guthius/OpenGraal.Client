#include "Actor.h"

#include <rlgl.h>

void Actor::Update(float dt)
{
	if (_animation != nullptr)
	{
		_animation->Update(dt, _animationState);
	}
}

void Actor::Draw() const
{
	if (_animation == nullptr)
	{
		return;
	}

	rlPushMatrix();
	rlTranslatef(-8, -16, 0);

	_animation->Draw(
			_position.x,
			_position.y,
			_dir,
			_animationState);

	rlPopMatrix();
}

void Actor::SetAnimation(const std::string &name)
{
	auto animationName = boost::to_lower_copy(name);

	if (animationName == _animationName)
	{
		return;
	}

	_animationName = animationName;
	_animation = AnimationManager::Get(_animationName);

	if (_animation == nullptr)
	{
		return;
	}

	_animationState.Reset(0, _animation);
}