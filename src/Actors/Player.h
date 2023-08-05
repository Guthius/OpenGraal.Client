#pragma once

#include "Actor.h"

class Player : public Actor
{
public:
	void Update(float dt) override;
};