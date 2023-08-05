#include "Player.h"

void Player::Update(float dt)
{
	const int speed = 4.0f;

	auto pos = GetPosition();

	bool moving = false;

	if (IsKeyDown(KEY_LEFT))
	{
		pos.x -= speed;
		SetDirection(Direction::Left);
		moving = true;
	}

	if (IsKeyDown(KEY_RIGHT))
	{
		pos.x += speed;
		SetDirection(Direction::Right);
		moving = true;
	}

	if (IsKeyDown(KEY_UP))
	{
		pos.y -= speed;
		SetDirection(Direction::Up);
		moving = true;
	}

	if (IsKeyDown(KEY_DOWN))
	{
		pos.y += speed;
		SetDirection(Direction::Down);
		moving = true;
	}

	if (moving)
	{
		SetAnimation("walk");
	}
	else
	{
		SetAnimation("idle");
	}

	auto mx = static_cast<float>(64 * 16 - 32);
	auto my = static_cast<float>(64 * 16 - 32);

	if (pos.x < 0) pos.x = 0;
	if (pos.y < 0) pos.y = 0;
	if (pos.x > mx) pos.x = mx;
	if (pos.y > my) pos.y = my;

	SetPosition(pos);

	Actor::Update(dt);
}
