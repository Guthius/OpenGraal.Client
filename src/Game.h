#pragma once

#include "Actor.h"
#include "Level.h"
#include "Player.h"
#include "Sign.h"
#include "Tileset.h"
#include "ThrownItem.h"

#include <vector>

#include "Leaps.h"

class Game
{
	struct LevelInfo
	{
		LevelInfo(Level *level, Tileset *tileset, const int x, const int y)
		{
			Level = level;
			Tileset = tileset;
			X = x;
			Y = y;
		}

		Level *Level;
		Tileset *Tileset;
		int X, Y;
	};

public:
	Game();

	void Run() const;

	void ChangeLevel(const std::string &level_name) const;
	auto GetCurrentLevel() const -> Level * { return level_->Level; }
	auto OnWall(Rectangle rect) const -> bool;
	auto OnWall(Vector2 pt) const -> bool;
	auto GetTileType(int x, int y) const -> int;
	void ShowSign(const std::string &str) const;
	void SpawnThrownItem(Actor::CarriedItem type, Vector2 origin, Direction dir) const;
	void SpawnLeaps(LeapType type, Vector2 origin) const;

private:
	void Update() const;
	void UpdateThrownItems(float dt) const;
	void UpdateLeaps(float dt) const;

	void Draw() const;
	void DrawPlayer() const;
	void DraWHud() const;
	void DrawHudResource(Rectangle rect, Vector2 pos, const std::string &text) const;
	void DrawDiagnostics() const;

	void DrawThrownItems() const;
	void DrawLeaps() const;


	Sign *sign_;
	LevelInfo *level_;
	Player *player_;
	Texture2D state_{};
	Font font20_{};
	Font font14_{};
	Font font_pixel_{};
	mutable std::vector<ThrownItem> thrown_items_{};
	mutable std::vector<Leap> leaps_{};
	mutable Texture2D sprites_{};
};
