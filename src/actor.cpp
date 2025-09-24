#include "actor.hpp"

#include <rlgl.h>
#include <boost/algorithm/string.hpp>

#include "texture_manager.hpp"

actor::actor() : sprites_(load_texture("sprites.png"))
{
	set_animation("idle");
}

void actor::update(const float dt)
{
	if (animation_ != nullptr)
	{
		animation_->update(dt, animation_state_);
	}
}

void actor::draw() const
{
	if (animation_ == nullptr)
	{
		return;
	}

	rlPushMatrix();
	rlTranslatef(-8, -16, 0);

	animation_->draw(
		position_.x,
		position_.y,
		dir_,
		animation_state_);

	rlPopMatrix();

	auto draw_carried_object_at = [&](const Vector2 src, const Vector2 dst) {
		DrawTextureRec(sprites_, {src.x, src.y, 32, 32}, dst, WHITE);
	};

	if (carried_object_ != carry_object_type::none && sprites_.id != 0)
	{
		Vector2 dest{position_.x, position_.y - 40};

		get_carried_destination_override(dest);

		switch (carried_object_)
		{
			case carry_object_type::bush:
				draw_carried_object_at({0.0f, 338.0f}, dest);
				break;

			case carry_object_type::sign:
				draw_carried_object_at({32.0f, 338.0f}, dest);
				break;

			case carry_object_type::vase:
				draw_carried_object_at({64.0f, 338.0f}, dest);
				break;

			case carry_object_type::stone:
				draw_carried_object_at({96.0f, 338.0f}, dest);
				break;

			case carry_object_type::black_stone:
				draw_carried_object_at({96.0f, 370.0f}, dest);
				break;

			default: break;
		}
	}
}

void actor::set_animation(const std::string &name)
{
	const auto animation_name = boost::to_lower_copy(name);

	if (animation_name == animation_name_)
	{
		return;
	}

	animation_name_ = animation_name;
	animation_ = load_animation(animation_name_);

	if (animation_ == nullptr)
	{
		return;
	}

	animation_state_.reset(0, animation_);
}
