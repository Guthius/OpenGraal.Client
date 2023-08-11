#pragma once

#include "Actor.h"

class Game;
class Player : public Actor
{
public:
	explicit Player(Game *game);

public:
	void Update(float dt) override;
	void Draw() const override;


private:
	void ReturnIdle();
	bool CheckForLevelLinkAt(Vector2 &position);
	bool CheckForSignAt(Vector2 &position);
	void CheckAttack(Vector2 &position);
	bool CheckMovement(Vector2 &position, float speed, float slideSpeed);
	void CheckPushAndPull();
	int CheckWall(int dir, float speed);
	void ClearGap(Vector2 &position, int dir, float speed);
	void Slide(Vector2 &position, int dir, float speed);
	void UpdateAnimation();
	int GetTileFacing();


	bool CheckJump(float dt, Vector2 &position);
	bool CanJump(Vector2 &position);
	void Jump();
	bool JumpUpdate(float dt, Vector2 &position);

	enum class Mode {
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

private:
	enum class OverlayType
	{
		Grass,
		GrassLava,
		Water,
		Lava,

		None
	};

	OverlayType _overlay = OverlayType::None;
	int _overlayFrame = 0;
	float _overlayTimer = 0.1f;

	void UpdateOverlay(float dt);
	void DrawOverlay() const;
	void SetOverlay(OverlayType overlay);

private:
	Game *_game;
	Mode _mode = Mode::Idle;
	float _speed = 4.0f;
	float _slideSpeed = 1.0f;
	Texture2D _sprites{};
	int _wall = 0;
	float _pushTimer = 0.0f;


	float _jumpTimer = 0.0f;
	int _jumpStep = 0;
	Vector2 _jumpOrigin{};
	Vector2 _jumpFrom{};
	Vector2 _jumpTo{};
	Sound _jumpSound{};
};