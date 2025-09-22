#include "ThrownItem.h"

#include "TextureManager.h"

constexpr auto carry_height = 40.0f;
constexpr auto travel_distance_in_tiles = 7.0f;
constexpr auto travel_distance = travel_distance_in_tiles * 16.0f;

static constexpr Rectangle GetSpriteRect(const Actor::CarriedItem type)
{
	switch (type)
	{
		case Actor::CarriedItem::Bush: return {0.0f, 338.0f, 32, 32};
		case Actor::CarriedItem::Sign: return {32.0f, 338.0f, 32, 32};
		case Actor::CarriedItem::Vase: return {64.0f, 338.0f, 32, 32};
		case Actor::CarriedItem::Stone: return {96.0f, 338.0f, 32, 32};
		case Actor::CarriedItem::BlackStone: return {96.0f, 370.0f, 32, 32};
		default: return {};
	}
}

ThrownItem::ThrownItem(const Actor::CarriedItem type, const Vector2 origin, const Direction dir)
	: type_(type), start_(origin), position_(origin), dir_(dir),
	  sprites_(TextureManager::Get("sprites.png"))
{
	const auto [dir_X, dir_y] = GetDirectionVector(dir);

	end_ = {
		origin.x + dir_X * travel_distance,
		origin.y + dir_y * travel_distance
	};

	if (dir == Direction::DIR_DOWN)
	{
		end_.y += carry_height;
	}
}

void ThrownItem::Update(const float dt)
{
	if (!alive_)
	{
		return;
	}

	time_elapsed_ += dt / duration_;
	if (time_elapsed_ >= 1.0f)
	{
		time_elapsed_ = 1.0f;
		alive_ = false;
	}

	position_.x = start_.x + (end_.x - start_.x) * time_elapsed_;
	position_.y = start_.y + (end_.y - start_.y) * time_elapsed_;

	if (dir_ == Direction::DIR_LEFT || dir_ == Direction::DIR_RIGHT)
	{
		const float drop = carry_height * (time_elapsed_ * time_elapsed_);

		position_.y += drop;
	}
}

void ThrownItem::Draw() const
{
	if (!IsAlive())
	{
		return;
	}

	const auto rect = GetSpriteRect(type_);
	if (rect.width == 0.0f || rect.height == 0.0f)
	{
		return;
	}

	DrawShadow();

	DrawTextureRec(sprites_, rect, position_, WHITE);
}

void ThrownItem::DrawShadow() const
{
	const auto sx = position_.x + 4;

	float sy = 0.0f;
	switch (dir_)
	{
		case Direction::DIR_LEFT:
		case Direction::DIR_RIGHT:
			sy = start_.y + carry_height + 16;
			break;

		case Direction::DIR_UP:
		case Direction::DIR_DOWN:
			{
				const auto shadow_start = start_.y + carry_height + 16.0f;
				const auto shadow_end = end_.y + 16.0f;
				const auto time = std::min<float>(time_elapsed_ * 1.0f, 1.0f);
				sy = shadow_start + (shadow_end - shadow_start) * time;
				break;
			}
	}

	DrawTextureRec(sprites_, {0, 0, 24, 12}, {sx, sy}, WHITE);
}
