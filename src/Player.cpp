#include "Player.h"
#include "Game.h"

#define BLOCK_TILE1 1
#define BLOCK_TILE2 2

static int movementKeys[4] = {KEY_UP, KEY_LEFT, KEY_DOWN, KEY_RIGHT};

static float vecx[4] = {0, -1, 0, 1};
static float vecy[4] = {-1, 0, 1, 0};

Player::Player(Game *game)
{
	_game = game;
	_sprites = TextureManager::Get("sprites.png");
}

void Player::Update(float dt)
{
	auto position = GetPosition();

	auto oldMode = _mode;

	ReturnIdle();

	if (CheckMovement(position, _speed, _slideSpeed))
	{
		SetPosition(position);
	}

	CheckForLevelLinkAt(position);

	if (_mode != oldMode)
	{
		switch (_mode)
		{
			case Mode::Idle:
				SetAnimation("idle");
				break;

			case Mode::Walk:
				SetAnimation("walk");
				break;

			case Mode::Swim:
				SetAnimation("swim");
				break;

			case Mode::Sit:
				SetAnimation("sit");
				break;

			default:
				break;
		}
	}

	Actor::Update(dt);

	UpdateOverlay(dt);
}

static float terrainSprite[][2] =
		{
				{0, 274},
				{32, 274},
				{0, 306},
				{32, 306},
		};

void Player::UpdateOverlay(float dt)
{
	if (_overlay == OverlayType::None)
	{
		return;
	}

	if (_overlay == OverlayType::Grass || _overlay == OverlayType::GrassLava)
	{
		if (_mode != Mode::Walk)
		{
			return;
		}
	}

	_overlayTimer -= dt;

	if (_overlayTimer <= 0)
	{
		_overlayTimer = 0.1f;
		_overlayFrame++;

		if (_overlayFrame > 1)
		{
			_overlayFrame = 0;
		}
	}
}

void Player::Draw() const
{
	Actor::Draw();

	DrawOverlay();
}

void Player::DrawOverlay() const
{
	if (_overlay == OverlayType::None)
	{
		return;
	}

	auto i = (int) _overlay;
	auto sx = terrainSprite[i][0];
	auto sy = terrainSprite[i][1];

	sy += static_cast<float>(_overlayFrame) * 16;

	auto pos = GetPosition();
	auto x = pos.x - 1;
	auto y = pos.y + 16 + 1;

	DrawTextureRec(_sprites, {sx, sy, 32, 16}, {x, y}, WHITE);
}

void Player::SetOverlay(OverlayType overlay)
{
	if (_overlay == overlay)
	{
		return;
	}

	_overlay = overlay;
	_overlayFrame = 0;
	_overlayTimer = 0.1f;
}

void Player::ReturnIdle()
{
	if (_mode == Mode::Swim)
	{
		return;
	}

	if ((_mode == Mode::Pull || _mode == Mode::Grab) && IsKeyDown(KEY_A))
	{
		return;
	}

	_mode = Mode::Idle;
}

bool Player::CheckForLevelLinkAt(Vector2 &position)
{
	auto dir = (int) GetDirection();
	auto x = static_cast<int>(position.x + 16 + (vecx[dir] * 18));
	auto y = static_cast<int>(position.y + 16 + (vecy[dir] * 18));

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

bool Player::CheckMovement(Vector2 &position, float speed, float slideSpeed)
{
	if (_mode == Mode::Grab || _mode == Mode::Pull)
	{
		return false;
	}

	auto moved = false;
	auto blocked = 0;

	for (int i = 0; i < 4; ++i)
	{
		if (!IsKeyDown(movementKeys[i]))
		{
			continue;
		}

		auto direction = (Direction) i;

		/*
    // IF PLAYER MOVES, STOP THE PLAYER FROM PUSHING ANYMORE
      if (k != player.dir) player.notpush = timevar2;
		 * */

		SetDirection(direction);

		blocked = CheckWall(direction, speed);

		if (blocked == 0)
		{
			position.x += vecx[i] * speed;
			position.y += vecy[i] * speed;
			moved = true;
		}
		else
		{
			ClearGap(position, direction, speed);

			if (blocked != (BLOCK_TILE1 | BLOCK_TILE2))
			{
				Slide(position, direction, blocked, slideSpeed);
			}
		}

		_mode = Mode::Walk;
	}

	auto cx = static_cast<int>(position.x + 16);
	auto cy = static_cast<int>(position.y + 16);

	auto floor = _game->GetTileType(cx, cy);

	if (floor & TileType::Chair)
	{
		SetOverlay(OverlayType::None);
		_mode = Mode::Sit;
	}
	else if (floor & TileType::Swamp)
	{
		SetOverlay(OverlayType::Grass);
	}
	else if (floor & TileType::WaterShallow)
	{
		SetOverlay(OverlayType::Water);
	}
	else if (floor & TileType::Water)
	{
		SetOverlay(OverlayType::None);
		_mode = Mode::Swim;
	}
	else
	{
		SetOverlay(OverlayType::None);
	}

	if (_mode == Mode::Swim || blocked != (BLOCK_TILE1 | BLOCK_TILE2))
	{
		return moved;
	}

	if (IsKeyDown(KEY_A))
	{
		if (IsKeyDown(movementKeys[0]))
		{
			_mode = Mode::Pull;
		}
		else
		{
			_mode = Mode::Grab;
		}
	}

	return moved;
}

int Player::CheckWall(Direction direction, float speed)
{
	auto pos = GetPosition();

	auto i = (int) direction;

	auto ax = pos.x + vecx[i] * (i < 2 ? speed : 32);
	auto ay = pos.y + vecy[i] * (i < 2 ? speed : 32);
	auto bx = pos.x + 16 + vecx[i] * (i < 2 ? speed + 16 : 16);
	auto by = pos.y + 16 + vecy[i] * (i < 2 ? speed + 16 : 16);

	auto w = ((i == 1 || i == 3) ? speed : 16);
	auto h = ((i == 0 || i == 2) ? speed : 16);

	char result = 0;

	if (_game->OnWall((Rectangle) {ax, ay, w - 1, h - 1}))
	{
		result |= BLOCK_TILE1;
	}

	if (_game->OnWall((Rectangle) {bx, by, w - 1, h - 1}))
	{
		result |= BLOCK_TILE2;
	}

	return result;
}

void Player::ClearGap(Vector2 &position, Direction direction, float speed)
{
	float dist = 0;

	for (; dist < speed;)
	{
		auto wall = CheckWall(direction, dist);

		if (wall != 0)
		{
			break;
		}

		dist++;
	}

	if (dist == 0)
	{
		return;
	}

	auto dir = (int) direction;

	switch (direction)
	{
		case Direction::Up:
		case Direction::Down:
			position.y += vecx[dir] * dist;
			break;

		case Direction::Left:
		case Direction::Right:
			position.x += vecy[dir] * dist;
			break;
	}
}

void Player::Slide(Vector2 &position, Direction direction, int wall, float slideSpeed)
{
	Direction slideDirection;

	if (wall == BLOCK_TILE1)
	{
		switch (direction)
		{
			case Direction::Up:
				slideDirection = Direction::Right;
				break;
			case Direction::Left:
				slideDirection = Direction::Down;
				break;
			case Direction::Down:
				slideDirection = Direction::Right;
				break;
			case Direction::Right:
				slideDirection = Direction::Down;
				break;
			default:
				return;
		}
	}
	else
	{
		switch (direction)
		{
			case Direction::Up:
				slideDirection = Direction::Left;
				break;
			case Direction::Left:
				slideDirection = Direction::Up;
				break;
			case Direction::Down:
				slideDirection = Direction::Left;
				break;
			case Direction::Right:
				slideDirection = Direction::Up;
				break;
			default:
				return;
		}
	}

	auto blocked = CheckWall(slideDirection, slideSpeed);

	if (blocked != 0)
	{
		return;
	}

	auto dir = (int) slideDirection;

	switch (direction)
	{
		case Direction::Up:
		case Direction::Down:
			position.x += vecx[dir] * slideSpeed;
			break;

		case Direction::Left:
		case Direction::Right:
			position.y += vecy[dir] * slideSpeed;
			break;
	}

	SetPosition(position);
}