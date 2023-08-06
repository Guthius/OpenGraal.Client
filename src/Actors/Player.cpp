#include "Player.h"
#include "../Game.h"

Player::Player(Game *game)
{
	_game = game;
}

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

	if (moving)
	{
		auto tx = pos.x;
		auto ty = pos.y;

		switch (GetDirection())
		{
			case Direction::Up:
				tx += 16;
				break;

			case Direction::Right:
				tx += 32;
				ty += 16;
				break;

			case Direction::Left:
				ty += 16;
				break;

			case Direction::Down:
				tx += 16;
				ty += 32;
				break;
		}

		if (!CheckForLevelLinkAt(tx, ty))
		{
			SetPosition(pos);
		}
	}

	Actor::Update(dt);
}

bool Player::CheckForLevelLinkAt(int x, int y)
{
	auto level = _game->GetCurrentLevel();
	if (level == nullptr)
	{
		return false;
	}

	auto link = level->GetLinkAt(x, y);
	if (link == nullptr)
	{
		return false;
	}

	auto dx = std::stof(link->GetNewX()) * 16 + 8;
	auto dy = std::stof(link->GetNewY()) * 16 + 16;
	auto pos = Vector2{dx, dy};

	SetPosition(pos);

	_game->ChangeLevel(link->GetNewLevel());

	return true;
}