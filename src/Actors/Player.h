#pragma once

#include "Actor.h"

class Game;
class Player : public Actor
{
public:
	explicit Player(Game *game);

public:
	void Update(float dt) override;

private:
	bool CheckForLevelLinkAt(int x, int y);

private:
	Game *_game;
};