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
	bool CheckMovement(Vector2 &position, float speed, float slideSpeed);
	int CheckWall(Direction direction, float speed);
	void ClearGap(Vector2 &position, Direction direction, float speed);
	void Slide(Vector2 &position, Direction direction, int wallCheck, float slideSpeed);

	enum class Mode {
		Idle,
		Walk,
		Grab,
		Pull,
		Swim,
		Sit
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
};