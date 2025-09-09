#include "Player.h"

#include <raymath.h>

#include "Game.h"
#include "SoundManager.h"
#include "TextureManager.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"
#pragma ide diagnostic ignored "ConstantConditionsOC"

#define BLOCK_TILE1 1
#define BLOCK_TILE2 2

constexpr int MovementKeys[4] = {KEY_UP, KEY_LEFT, KEY_DOWN, KEY_RIGHT};

static constexpr float VX[4] = {0, -1, 0, 1};
static constexpr float VY[4] = {-1, 0, 1, 0};

static constexpr float JumpSpeed = 0.05f;
static constexpr Vector2 JumpFrames[][8] =
{
	{
		{0, -24},
		{0, -44},
		{0, -61},
		{0, -72},
		{0, -80},
		{0, -83},
		{0, -83},
		{0, -80}
	},
	{
		{-16, -3},
		{-32, -8},
		{-48, 0},
		{-61, 8},
		{-72, 18},
		{-83, 27},
		{-93, 35},
		{-104, 48}
	},
	{
		{0, -3},
		{0, 3},
		{0, 8},
		{0, 24},
		{0, 45},
		{0, 64},
		{0, 86},
		{0, 112}
	},
	{
		{16, -3},
		{32, -8},
		{48, 0},
		{61, 8},
		{72, 18},
		{83, 27},
		{93, 35},
		{104, 48}
	}
};

static constexpr auto GetDirectionKey(const int direction) -> int
{
	return MovementKeys[direction];
}

static constexpr auto GetOppositeDirectionKey(const int direction) -> int
{
	switch (direction)
	{
		default:
			return direction;
		case DIR_UP:
			return MovementKeys[2];
		case DIR_LEFT:
			return MovementKeys[3];
		case DIR_DOWN:
			return MovementKeys[0];
		case DIR_RIGHT:
			return MovementKeys[1];
	}
}

Player::Player(Game *game)
{
	game_ = game;
	sprites_ = TextureManager::Get("sprites.png");
	jump_sound_ = SoundManager::Get("jump.wav");
}

static constexpr auto GetSpeed(const float dt, const float speed) -> float
{
	return dt * speed * 70;
}

void Player::Update(const float dt)
{
	auto position = GetPosition();

	const auto mode = mode_;
	const auto speed = GetSpeed(dt, speed_);

	if (const auto slide_speed = GetSpeed(dt, slide_speed_);
		CheckJump(dt, position) ||
		CheckMovement(position, speed, slide_speed))
	{
		SetPosition(position);
	}

	CheckAttack(position);

	if (mode_ == Mode::Walk)
	{
		if (CheckForSignAt(position))
		{
			mode_ = Mode::Idle;
		}
	}

	CheckForLevelLinkAt(position);
	CheckPushAndPull();

	if (mode_ != mode)
	{
		UpdateAnimation();
	}

	Actor::Update(dt);

	UpdateOverlay(dt);
}

static float terrain_sprite[][2] =
{
	{0, 274},
	{32, 274},
	{0, 306},
	{32, 306},
};

void Player::Draw() const
{
	Actor::Draw();

	DrawOverlay();
}

void Player::ReturnIdle()
{
	if (mode_ == Mode::Attack)
	{
		if (!GetAnimationState().Ended)
		{
			return;
		}
	}

	if (mode_ == Mode::Swim)
	{
		return;
	}

	if ((mode_ == Mode::Pull || mode_ == Mode::Grab) && IsKeyDown(KEY_A))
	{
		return;
	}

	mode_ = Mode::Idle;
}

auto Player::CheckForLevelLinkAt(const Vector2 &position) -> bool
{
	const auto dir = GetDirection();
	const auto x = static_cast<int>(position.x + 16 + VX[dir] * 24);
	const auto y = static_cast<int>(position.y + 16 + VY[dir] * 24);

	const auto level = game_->GetCurrentLevel();
	if (level == nullptr)
	{
		return false;
	}

	const auto link = level->GetLinkAt(x, y);
	if (link == nullptr)
	{
		return false;
	}

	const auto dx = link->GetNewX() == "playerx" ? position.x : std::stof(link->GetNewX()) * 16 + 8;
	const auto dy = link->GetNewY() == "playery" ? position.y : std::stof(link->GetNewY()) * 16 + 16;

	const auto pos = Vector2{dx, dy};

	SetPosition(pos);

	game_->ChangeLevel(link->GetNewLevel());

	return true;
}

auto Player::CheckForSignAt(const Vector2 &position) const -> bool
{
	if (wall_ != (BLOCK_TILE1 | BLOCK_TILE2))
	{
		return false;
	}

	const auto dir = GetDirection();
	const auto x = static_cast<int>(position.x + 16 + VX[dir] * 24);
	const auto y = static_cast<int>(position.y + 16 + VY[dir] * 24);

	const auto level = game_->GetCurrentLevel();
	if (level == nullptr)
	{
		return false;
	}

	const auto sign = level->GetSignAt(x, y);
	if (sign == nullptr)
	{
		return false;
	}

	game_->ShowSign(sign->GetText());

	return true;
}

void Player::CheckAttack(Vector2 &position)
{
	if (mode_ != Mode::Walk && mode_ != Mode::Idle)
	{
		return;
	}

	if (!IsKeyPressed(KEY_S))
	{
		return;
	}

	mode_ = Mode::Attack;
}

auto Player::CheckMovement(Vector2 &position, const float speed, const float slide_speed) -> bool
{
	ReturnIdle();

	if (mode_ == Mode::Attack)
	{
		return false;
	}

	if (mode_ == Mode::Grab || mode_ == Mode::Pull)
	{
		return false;
	}

	auto moved = false;

	for (int dir = 0; dir < 4; ++dir)
	{
		if (!IsKeyDown(MovementKeys[dir]))
		{
			continue;
		}

		if (dir != GetDirection())
		{
			push_timer_ = 0;
		}

		SetDirection(dir);

		wall_ = CheckWall(dir, speed);

		if (wall_ == 0)
		{
			position.x += VX[dir] * speed;
			position.y += VY[dir] * speed;
			moved = true;
		}
		else
		{
			ClearGap(position, dir, speed);

			if (wall_ != (BLOCK_TILE1 | BLOCK_TILE2))
			{
				Slide(position, dir, slide_speed);
			}
		}

		mode_ = Mode::Walk;
	}

	const auto cx = static_cast<int>(position.x + 16);
	const auto cy = static_cast<int>(position.y + 16);

	if (const auto tile_type = game_->GetTileType(cx, cy); tile_type & TileType::Chair)
	{
		SetOverlay(OverlayType::None);

		mode_ = Mode::Sit;
	}
	else if (tile_type & TileType::Swamp)
	{
		SetOverlay(OverlayType::Grass);
	}
	else if (tile_type & TileType::WaterShallow)
	{
		SetOverlay(OverlayType::Water);
	}
	else if (tile_type & TileType::Water)
	{
		SetOverlay(OverlayType::None);

		mode_ = Mode::Swim;
	}
	else
	{
		SetOverlay(OverlayType::None);
	}

	return moved;
}

void Player::CheckPushAndPull()
{
	if (mode_ == Mode::Attack)
	{
		return;
	}

	if (mode_ == Mode::Jump)
	{
		return;
	}

	if (mode_ == Mode::Swim || wall_ != (BLOCK_TILE1 | BLOCK_TILE2))
	{
		return;
	}

	const auto dir = GetDirection();

	if (IsKeyDown(KEY_A))
	{
		if (IsKeyDown(GetOppositeDirectionKey(dir)))
		{
			mode_ = Mode::Pull;
		}
		else
		{
			mode_ = Mode::Grab;
		}
	}

	if (IsKeyDown(GetDirectionKey(dir)))
	{
		if (mode_ != Mode::Push)
		{
			push_timer_ += GetFrameTime();

			if (push_timer_ >= .75f)
			{
				mode_ = Mode::Push;
			}
		}
	}
	else
	{
		push_timer_ = 0;
	}
}

auto Player::CheckWall(const int dir, const float speed) const -> int
{
	const auto pos = GetPosition();

	const auto ax = pos.x + VX[dir] * (dir < 2 ? speed : 32);
	const auto ay = pos.y + VY[dir] * (dir < 2 ? speed : 32);
	const auto bx = pos.x + 16 + VX[dir] * (dir < 2 ? speed + 16 : 16);
	const auto by = pos.y + 16 + VY[dir] * (dir < 2 ? speed + 16 : 16);

	const auto w = dir == 1 || dir == 3 ? speed : 16;
	const auto h = dir == 0 || dir == 2 ? speed : 16;

	char result = 0;

	if (game_->OnWall((Rectangle){ax, ay, w - 1, h - 1}))
	{
		result |= BLOCK_TILE1;
	}

	if (game_->OnWall((Rectangle){bx, by, w - 1, h - 1}))
	{
		result |= BLOCK_TILE2;
	}

	return result;
}

void Player::ClearGap(Vector2 &position, const int dir, const float speed) const
{
	float dist = 0;

	while (dist < speed)
	{
		if (const auto wall = CheckWall(dir, dist); wall != 0)
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
			position.y += VX[dir] * dist;
			break;

		case DIR_LEFT:
		case DIR_RIGHT:
			position.x += VY[dir] * dist;
			break;
	}
}

void Player::Slide(Vector2 &position, const int dir, const float speed)
{
	int slide_dir;

	if (wall_ == BLOCK_TILE1)
	{
		switch (dir)
		{
			case DIR_UP:
				slide_dir = DIR_RIGHT;
				break;
			case DIR_LEFT:
				slide_dir = DIR_DOWN;
				break;
			case DIR_DOWN:
				slide_dir = DIR_RIGHT;
				break;
			case DIR_RIGHT:
				slide_dir = DIR_DOWN;
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
				slide_dir = DIR_LEFT;
				break;
			case DIR_LEFT:
				slide_dir = DIR_UP;
				break;
			case DIR_DOWN:
				slide_dir = DIR_LEFT;
				break;
			case DIR_RIGHT:
				slide_dir = DIR_UP;
				break;
			default:
				return;
		}
	}

	if (const auto blocked = CheckWall(slide_dir, speed); blocked != 0)
	{
		return;
	}

	switch (dir)
	{
		case DIR_UP:
		case DIR_DOWN:
			position.x += VX[slide_dir] * speed;
			break;

		case DIR_LEFT:
		case DIR_RIGHT:
			position.y += VY[slide_dir] * speed;
			break;
	}

	SetPosition(position);
}

void Player::UpdateAnimation()
{
	switch (mode_)
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

		case Mode::Attack:
			SetAnimation("sword");
			break;

		default:
			break;
	}
}

auto Player::GetTileFacing() const -> int
{
	const auto position = GetPosition();
	const auto dir = GetDirection();
	const auto x = static_cast<int>(position.x + 16 + VX[dir] * 24);
	const auto y = static_cast<int>(position.y + 16 + VY[dir] * 24);

	return game_->GetTileType(x, y);
}

bool Player::CheckJump(const float dt, Vector2 &position)
{
	if (mode_ == Mode::Jump)
	{
		return JumpUpdate(dt, position);
	}

	if (!CanJump(position))
	{
		return false;
	}

	Jump();

	return true;
}

auto Player::CanJump(const Vector2 &position) const -> bool
{
	if (mode_ != Mode::Push)
	{
		return false;
	}

	if (const auto tile = GetTileFacing(); !(tile & TileType::Jump))
	{
		return false;
	}

	const auto dir = GetDirection();
	const auto x = position.x + JumpFrames[dir][7].x;
	const auto y = position.y + JumpFrames[dir][7].y;

	if (game_->OnWall({x, y, 31, 31}))
	{
		return false;
	}

	return true;
}

void Player::Jump()
{
	const auto dir = GetDirection();

	SetAnimation("walk");

	PlaySound(jump_sound_);

	mode_ = Mode::Jump;
	jump_step_ = 0;
	jump_timer_ = 0;
	_jumpOrigin = GetPosition();
	jump_from_ = _jumpOrigin;
	jump_to_.x = jump_from_.x + JumpFrames[dir][jump_step_].x;
	jump_to_.y = jump_from_.y + JumpFrames[dir][jump_step_].y;
}

auto Player::JumpUpdate(const float dt, Vector2 &position) -> bool
{
	jump_timer_ += dt;

	if (jump_timer_ >= JumpSpeed)
	{
		position = jump_to_;

		jump_timer_ = 0;
		jump_step_++;

		if (jump_step_ >= 8)
		{
			mode_ = Mode::Idle;

			return false;
		}

		const auto dir = GetDirection();

		jump_from_ = GetPosition();
		jump_to_.x = _jumpOrigin.x + JumpFrames[dir][jump_step_].x;
		jump_to_.y = _jumpOrigin.y + JumpFrames[dir][jump_step_].y;
	}

	position = Vector2Lerp(jump_from_, jump_to_, jump_timer_ / JumpSpeed);

	return true;
}

void Player::UpdateOverlay(const float dt)
{
	if (overlay_ == OverlayType::None)
	{
		return;
	}

	if (overlay_ == OverlayType::Grass ||
	    overlay_ == OverlayType::GrassLava)
	{
		if (mode_ != Mode::Walk)
		{
			return;
		}
	}

	overlay_timer_ -= dt;

	if (overlay_timer_ <= 0)
	{
		overlay_timer_ = 0.1f;
		overlay_frame_++;

		if (overlay_frame_ > 1)
		{
			overlay_frame_ = 0;
		}
	}
}

void Player::DrawOverlay() const
{
	if (overlay_ == OverlayType::None)
	{
		return;
	}

	const auto i = static_cast<int>(overlay_);
	const auto sx = terrain_sprite[i][0];
	auto sy = terrain_sprite[i][1];

	sy += static_cast<float>(overlay_frame_) * 16;

	const auto pos = GetPosition();
	const auto x = pos.x - 1;
	const auto y = pos.y + 16 + 1;

	DrawTextureRec(sprites_, {sx, sy, 32, 16}, {x, y}, WHITE);
}

void Player::SetOverlay(const OverlayType overlay)
{
	if (overlay_ == overlay)
	{
		return;
	}

	overlay_ = overlay;
	overlay_frame_ = 0;
	overlay_timer_ = 0.1f;
}

#pragma clang diagnostic pop
