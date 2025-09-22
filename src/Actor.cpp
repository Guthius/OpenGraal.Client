#include "Actor.h"

#include <rlgl.h>
#include <boost/algorithm/string.hpp>

#include "TextureManager.h"

void Actor::Update(const float dt)
{
	if (sprites_.id == 0)
	{
		sprites_ = TextureManager::Get("sprites.png");
	}

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

	auto draw_carried_object_at = [&](const Vector2 src, const Vector2 dst) {
		DrawTextureRec(sprites_, {src.x, src.y, 32, 32}, dst, WHITE);
	};

	if (carried_object_ != CarriedItem::None && sprites_.id != 0)
	{
		const Vector2 dest{position_.x, position_.y - 40};

		switch (carried_object_)
		{
			case CarriedItem::Bush:
				draw_carried_object_at({0.0f, 338.0f}, dest);
				break;

			case CarriedItem::Sign:
				draw_carried_object_at({32.0f, 338.0f}, dest);
				break;

			case CarriedItem::Vase:
				draw_carried_object_at({64.0f, 338.0f}, dest);
				break;

			case CarriedItem::Stone:
				draw_carried_object_at({96.0f, 338.0f}, dest);
				break;

			case CarriedItem::BlackStone:
				draw_carried_object_at({96.0f, 370.0f}, dest);
				break;

			default: break;
		}
	}
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
