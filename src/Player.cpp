#include "Player.h"
#include "Game.h"

#define BLOCK_TILE1 1
#define BLOCK_TILE2 2

constexpr int movementKeys[4] = {KEY_UP, KEY_LEFT, KEY_DOWN, KEY_RIGHT};

static float vecx[4] = {0, -1, 0, 1};
static float vecy[4] = {-1, 0, 1, 0};

static constexpr int GetDirectionKey(int direction)
{
	return movementKeys[direction];
}

static constexpr int GetOppositeDirectionKey(int direction)
{
	switch (direction)
	{
		default:
			return direction;
		case DIR_UP:
			return movementKeys[2];
		case DIR_LEFT:
			return movementKeys[3];
		case DIR_DOWN:
			return movementKeys[0];
		case DIR_RIGHT:
			return movementKeys[1];
	}
}

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
	CheckPushAndPull();

	if (_mode != oldMode)
	{
		UpdateAnimation();
	}

	Actor::Update(dt);

	UpdateOverlay(dt);
}

static float terrainSprite[][2] =
		{
				{0,  274},
				{32, 274},
				{0,  306},
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

	for (int dir = 0; dir < 4; ++dir)
	{
		if (!IsKeyDown(movementKeys[dir]))
		{
			continue;
		}

		if (dir != GetDirection())
		{
			_pushTimer = 0;
		}

		SetDirection(dir);

		_wall = CheckWall(dir, speed);

		if (_wall == 0)
		{
			position.x += vecx[dir] * speed;
			position.y += vecy[dir] * speed;
			moved = true;
		}
		else
		{
			ClearGap(position, dir, speed);

			if (_wall != (BLOCK_TILE1 | BLOCK_TILE2))
			{
				Slide(position, dir, slideSpeed);
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


	return moved;
}

void Player::CheckPushAndPull()
{
	if (_mode == Mode::Swim || _wall != (BLOCK_TILE1 | BLOCK_TILE2))
	{
		return;
	}

	auto dir = GetDirection();

	if (IsKeyDown(KEY_A))
	{
		if (IsKeyDown(GetOppositeDirectionKey(dir)))
		{
			_mode = Mode::Pull;
		}
		else
		{
			_mode = Mode::Grab;
		}
	}

	if (IsKeyDown(GetDirectionKey(dir)))
	{
		if (_mode != Mode::Push)
		{
			_pushTimer += GetFrameTime();

			if (_pushTimer >= .75f)
			{
				_mode = Mode::Push;
			}
		}
	}
	else
	{
		_pushTimer = 0;
	}
}

int Player::CheckWall(int dir, float speed)
{
	auto pos = GetPosition();

	auto ax = pos.x + vecx[dir] * (dir < 2 ? speed : 32);
	auto ay = pos.y + vecy[dir] * (dir < 2 ? speed : 32);
	auto bx = pos.x + 16 + vecx[dir] * (dir < 2 ? speed + 16 : 16);
	auto by = pos.y + 16 + vecy[dir] * (dir < 2 ? speed + 16 : 16);

	auto w = ((dir == 1 || dir == 3) ? speed : 16);
	auto h = ((dir == 0 || dir == 2) ? speed : 16);

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

void Player::ClearGap(Vector2 &position, int dir, float speed)
{
	float dist = 0;

	for (; dist < speed;)
	{
		auto wall = CheckWall(dir, dist);

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

	switch (dir)
	{
		default:
			break;

		case DIR_UP:
		case DIR_DOWN:
			position.y += vecx[dir] * dist;
			break;

		case DIR_LEFT:
		case DIR_RIGHT:
			position.x += vecy[dir] * dist;
			break;
	}
}

void Player::Slide(Vector2 &position, int dir, float speed)
{
	int slideDir;

	if (_wall == BLOCK_TILE1)
	{
		switch (dir)
		{
			case DIR_UP:
				slideDir = DIR_RIGHT;
				break;
			case DIR_LEFT:
				slideDir = DIR_DOWN;
				break;
			case DIR_DOWN:
				slideDir = DIR_RIGHT;
				break;
			case DIR_RIGHT:
				slideDir = DIR_DOWN;
				break;
			default:
				return;
		}
	}
	else
	{
		switch (dir)
		{
			case DIR_UP:
				slideDir = DIR_LEFT;
				break;
			case DIR_LEFT:
				slideDir = DIR_UP;
				break;
			case DIR_DOWN:
				slideDir = DIR_LEFT;
				break;
			case DIR_RIGHT:
				slideDir = DIR_UP;
				break;
			default:
				return;
		}
	}

	auto blocked = CheckWall(slideDir, speed);

	if (blocked != 0)
	{
		return;
	}

	switch (dir)
	{
		default:
			return;

		case DIR_UP:
		case DIR_DOWN:
			position.x += vecx[slideDir] * speed;
			break;

		case DIR_LEFT:
		case DIR_RIGHT:
			position.y += vecy[slideDir] * speed;
			break;
	}

	SetPosition(position);
}

void Player::UpdateAnimation()
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

		case Mode::Pull:
			SetAnimation("pull");
			break;

		case Mode::Grab:
			SetAnimation("grab");
			break;

		case Mode::Push:
			SetAnimation("push");
			break;

		default:
			break;
	}
}