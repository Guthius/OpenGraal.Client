#include "Actor.h"

#include <rlgl.h>
#include <boost/algorithm/string.hpp>

void Actor::Update(const float dt)
{
	if (animation_ != nullptr)
	{
		animation_->Update(dt, animation_state_);
	}
}

void Actor::Draw() const
{
	if (animation_ == nullptr)
	{
		return;
	}

	rlPushMatrix();
	rlTranslatef(-8, -16, 0);

	animation_->Draw(
		position_.x,
		position_.y,
		dir_,
		animation_state_);

	rlPopMatrix();
}

void Actor::SetAnimation(const std::string &name)
{
	const auto animation_name = boost::to_lower_copy(name);

	if (animation_name == animation_name_)
	{
		return;
	}

	animation_name_ = animation_name;
	animation_ = AnimationManager::Get(animation_name_);

	if (animation_ == nullptr)
	{
		return;
	}

	animation_state_.Reset(0, animation_);
}
