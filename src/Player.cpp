#include "Player.h"

#include <cassert>
#include <raymath.h>

#include "Game.h"
#include "SoundManager.h"
#include "TextureManager.h"

static constexpr int Corner1 = 1;
static constexpr int Corner2 = 2;

static auto IsGridAligned(const Vector2 &pos) -> bool
{
	static constexpr float grid_alignment_epsilon = 0.001f; // tolerance for float rounding issues

	const auto is_near = [](const float v, const float eps) {
		return std::fabs(v - std::round(v)) <= eps;
	};

	return is_near(pos.x, grid_alignment_epsilon) &&
	       is_near(pos.y, grid_alignment_epsilon);
}

static constexpr auto GetDirectionVector(const Direction direction) -> Vector2
{
	switch (direction)
	{
		case Direction::DIR_UP:
			return {0, -1};
		case Direction::DIR_LEFT:
			return {-1, 0};
		case Direction::DIR_DOWN:
			return {0, 1};
		case Direction::DIR_RIGHT:
			return {1, 0};
		default:
			return {};
	}
}

static constexpr float jump_speed = 0.05f;
static constexpr Vector2 jump_frames[][8] =
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

static constexpr auto GetDirectionKey(const Direction direction) -> int
{
	switch (direction)
	{
		case Direction::DIR_UP:
			return KEY_UP;
		case Direction::DIR_LEFT:
			return KEY_LEFT;
		case Direction::DIR_DOWN:
			return KEY_DOWN;
		case Direction::DIR_RIGHT:
			return KEY_RIGHT;
		default:
			return KEY_NULL;
	}
}

static constexpr auto GetOppositeDirectionKey(const Direction direction) -> int
{
	switch (direction)
	{
		case Direction::DIR_UP:
			return KEY_DOWN;
		case Direction::DIR_LEFT:
			return KEY_RIGHT;
		case Direction::DIR_DOWN:
			return KEY_UP;
		case Direction::DIR_RIGHT:
			return KEY_LEFT;
		default:
			return KEY_NULL;
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

	const auto [dirx, diry] = GetDirectionVector(dir);
	const auto x = static_cast<int>(position.x + 16 + dirx * 17);
	const auto y = static_cast<int>(position.y + 16 + diry * 17);

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

	const auto dx = link->GetNewX() == "playerx" ? position.x : (std::stof(link->GetNewX()) + 0.5f) * 16;
	const auto dy = link->GetNewY() == "playery" ? position.y : (std::stof(link->GetNewY()) + 1.0f) * 16;

	TraceLog(LOG_INFO, "Warp to %s @ %s, %s (%f, %f)",
	         link->GetNewLevel().c_str(),
	         link->GetNewX().c_str(),
	         link->GetNewY().c_str(),
	         dx, dy);

	const auto pos = Vector2{dx, dy};

	SetPosition(pos);

	game_->ChangeLevel(link->GetNewLevel());

	return true;
}

auto Player::CheckForSignAt(const Vector2 &position) const -> bool
{
	if (!IsFacingWall())
	{
		return false;
	}

	const auto dir = GetDirection();

	const auto [dirx, diry] = GetDirectionVector(dir);
	const auto x = static_cast<int>(position.x + 16 + dirx * 24);
	const auto y = static_cast<int>(position.y + 16 + diry * 24);

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
	static constexpr Direction directions[] = {
		Direction::DIR_UP,
		Direction::DIR_LEFT,
		Direction::DIR_DOWN,
		Direction::DIR_RIGHT,
	};

	ReturnIdle();

	if (mode_ == Mode::Attack)
	{
		return false;
	}

	if (mode_ == Mode::Grab || mode_ == Mode::Pull)
	{
		return false;
	}

	bool moved = false;

	Vector2 move_vector = {0, 0};

	for (const auto dir: directions)
	{
		if (IsKeyDown(GetDirectionKey(dir)))
		{
			move_vector += GetDirectionVector(dir);

			if (dir != GetDirection())
			{
				push_timer_ = 0;
			}

			SetDirection(dir);
		}
	}

	move_vector = Vector2Normalize(move_vector);
	if (move_vector.x != 0 || move_vector.y != 0)
	{
		if ((moved = TryMove(position, move_vector, speed)))
		{
			mode_ = Mode::Walk;
		}
	}

	// Attempt to slide along corners only if we tried to move but didn't,
	// the player is facing a wall corner, and we're not perfectly grid-aligned.
	if (!moved)
	{
		if (const auto wall = CheckWall(GetDirection());
			IsKeyDown(GetDirectionKey(GetDirection())) &&
			(wall == Corner1 || wall == Corner2) &&
			!IsGridAligned(position))
		{
			const auto [x, y] = position;

			Slide(position, GetDirection(), wall, slide_speed);

			if (position.x != x || position.y != y)
			{
				moved = true;
				mode_ = Mode::Walk;
			}
		}
	}

	const auto check_x = static_cast<int>(position.x + 16);
	const auto check_y = static_cast<int>(position.y + 24);

	if (const auto tile_type = game_->GetTileType(check_x, check_y); tile_type & TileType::Chair)
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

auto Player::TryMove(Vector2 &position, const Vector2 direction, const float speed) const -> bool
{
	auto collides = [&](const float nx, const float ny) -> bool {
		return game_->OnWall(Rectangle{nx, ny, 31.0f, 31.0f});
	};

	auto check_x = [&](float &x, const float delta) {
		const float step_sign = delta > 0.0f ? 1.0f : -1.0f;
		auto remaining = std::fabs(delta);
		while (remaining > 0.0f)
		{
			const float step = std::min(1.0f, remaining);

			if (collides(x + step_sign * step, position.y))
			{
				break;
			}

			x += step_sign * step;

			remaining -= step;
		}
	};

	auto check_y = [&](float &y, const float delta) {
		const float step_sign = delta > 0.0f ? 1.0f : -1.0f;
		auto remaining = std::fabs(delta);
		while (remaining > 0.0f)
		{
			const float step = std::min(1.0f, remaining);

			if (collides(position.x, y + step_sign * step))
			{
				break;
			}

			y += step_sign * step;

			remaining -= step;
		}
	};

	const auto start_x = position.x;
	const auto start_y = position.y;

	const auto delta_x = direction.x * speed;
	const auto delta_y = direction.y * speed;

	if (delta_x != 0.0f) check_x(position.x, delta_x);
	if (delta_y != 0.0f) check_y(position.y, delta_y);

	return position.x != start_x ||
	       position.y != start_y;
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

	if (mode_ == Mode::Swim || !IsFacingWall())
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

auto Player::CheckWall(const Direction dir) const -> int
{
	auto [x, y] = GetPosition();

	// x += 8;
	// y += 16;

	Vector2 v1, v2;
	switch (dir)
	{
		case Direction::DIR_UP:
			v1 = {x, y - 1};
			v2 = {x + 31, y - 1};
			break;

		case Direction::DIR_LEFT:
			v1 = {x - 1, y};
			v2 = {x - 1, y + 31};
			break;

		case Direction::DIR_DOWN:
			v1 = {x + 1, y + 32};
			v2 = {x + 31, y + 32};
			break;

		case Direction::DIR_RIGHT:
			v1 = {x + 32, y};
			v2 = {x + 32, y + 31};
			break;

		default:
			return 0;
	}

	char result = 0;

	if (game_->OnWall(v1)) result |= Corner1;
	if (game_->OnWall(v2)) result |= Corner2;

	return result;
}

void Player::Slide(Vector2 &position, const Direction dir, int wall, const float speed)
{
	if (IsGridAligned(position))
	{
		return;
	}

	Direction slide_dir;

	if (wall == Corner1)
	{
		switch (dir)
		{
			case Direction::DIR_UP:
				slide_dir = Direction::DIR_RIGHT;
				break;
			case Direction::DIR_LEFT:
				slide_dir = Direction::DIR_DOWN;
				break;
			case Direction::DIR_DOWN:
				slide_dir = Direction::DIR_RIGHT;
				break;
			case Direction::DIR_RIGHT:
				slide_dir = Direction::DIR_DOWN;
				break;
			default:
				return;
		}
	}
	else
	{
		switch (dir)
		{
			case Direction::DIR_UP:
				slide_dir = Direction::DIR_LEFT;
				break;
			case Direction::DIR_LEFT:
				slide_dir = Direction::DIR_UP;
				break;
			case Direction::DIR_DOWN:
				slide_dir = Direction::DIR_LEFT;
				break;
			case Direction::DIR_RIGHT:
				slide_dir = Direction::DIR_UP;
				break;
			default:
				return;
		}
	}

	if (const auto blocked = CheckWall(slide_dir); blocked != 0)
	{
		return;
	}

	const auto [dirx, diry] = GetDirectionVector(slide_dir);

	position.x += std::min(dirx * speed, 1.f);
	position.y += std::min(diry * speed, 1.f);

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
	const auto [x, y] = GetPosition();
	const auto [dirx, diry] = GetDirectionVector(GetDirection());

	const auto lx = static_cast<int>(x + 16 + dirx * 24);
	const auto ly = static_cast<int>(y + 16 + diry * 24);

	return game_->GetTileType(lx, ly);
}

auto Player::IsFacingWall() const -> int
{
	auto [x, y] = GetPosition();

	Vector2 v1, v2;

	switch (GetDirection())
	{
		default: return false;
		case Direction::DIR_UP:
			v1 = {x + 8, y - 1};
			v2 = {x + 31 - 8, y - 1};
			break;

		case Direction::DIR_LEFT:
			v1 = {x - 1, y + 8};
			v2 = {x - 1, y + 31 - 8};
			break;

		case Direction::DIR_DOWN:
			v1 = {x + 8, y + 32};
			v2 = {x + 31 - 8, y + 32};
			break;

		case Direction::DIR_RIGHT:
			v1 = {x + 32, y + 8};
			v2 = {x + 32, y + 31 - 8};
			break;
	}

	return game_->OnWall(v1) || game_->OnWall(v2);
}

auto Player::CheckJump(const float dt, Vector2 &position) -> bool
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
	const auto x = position.x + jump_frames[static_cast<int>(dir)][7].x;
	const auto y = position.y + jump_frames[static_cast<int>(dir)][7].y;

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
	jump_origin_ = GetPosition();
	jump_from_ = jump_origin_;
	jump_to_.x = jump_from_.x + jump_frames[static_cast<int>(dir)][jump_step_].x;
	jump_to_.y = jump_from_.y + jump_frames[static_cast<int>(dir)][jump_step_].y;
}

auto Player::JumpUpdate(const float dt, Vector2 &position) -> bool
{
	jump_timer_ += dt;

	if (jump_timer_ >= jump_speed)
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
		jump_to_.x = jump_origin_.x + jump_frames[static_cast<int>(dir)][jump_step_].x;
		jump_to_.y = jump_origin_.y + jump_frames[static_cast<int>(dir)][jump_step_].y;
	}

	position = Vector2Lerp(jump_from_, jump_to_, jump_timer_ / jump_speed);

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

	const auto [x, y] = GetPosition();
	const auto dx = x;
	const auto dy = y + 16;

	DrawTextureRec(sprites_, {sx, sy, 32, 16}, {dx, dy}, WHITE);
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
