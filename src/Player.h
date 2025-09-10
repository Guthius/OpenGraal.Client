#pragma once

#include "Actor.h"

class Game;

class Player final : public Actor
{
public:
	explicit Player(Game *game);

	void Update(float dt) override;
	void Draw() const override;

private:
	void ReturnIdle();
	auto CheckForLevelLinkAt(const Vector2 &position) -> bool;
	auto CheckForSignAt(const Vector2 &position) const -> bool;
	void CheckAttack(Vector2 &position);
	auto CheckMovement(Vector2 &position, float speed, float slide_speed) -> bool;
	auto TryMove(Vector2& position, Vector2 direction, float speed) const -> bool;
	auto TryMoveFromWall(Vector2 position) -> void;
	void CheckPushAndPull();
	auto CheckWall(Direction dir) const -> int;
	void Slide(Vector2 &position, Direction dir, int wall, float speed);
	void UpdateAnimation();
	auto GetTileFacing() const -> int;
	auto IsFacingWall() const -> int;

	auto CheckJump(float dt, Vector2 &position) -> bool;
	auto CanJump(const Vector2 &position) const -> bool;
	void Jump();
	auto JumpUpdate(float dt, Vector2 &position) -> bool;

	enum class Mode
	{
		Idle,
		Walk,
		Grab,
		Push,
		Pull,
		Swim,
		Sit,
		Jump,
		Attack
	};

	enum class OverlayType
	{
		Grass,
		GrassLava,
		Water,
		Lava,

		None
	};

	OverlayType overlay_ = OverlayType::None;
	int overlay_frame_ = 0;
	float overlay_timer_ = 0.1f;

	void UpdateOverlay(float dt);
	void DrawOverlay() const;

	void SetOverlay(OverlayType overlay);

	Game *game_;
	Mode mode_ = Mode::Idle;
	float speed_ = 4.0f;
	float slide_speed_ = 1.0f;
	Texture2D sprites_{};
	float push_timer_ = 0.0f;
	float jump_timer_ = 0.0f;
	int jump_step_ = 0;
	Vector2 jump_origin_{};
	Vector2 jump_from_{};
	Vector2 jump_to_{};
	Sound jump_sound_{};
};
