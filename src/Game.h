#pragma once

#include "Actor.h"
#include "Level.h"
#include "Player.h"
#include "Sign.h"
#include "Tileset.h"

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

private:
	void Update() const;

	void Draw() const;
	void DrawPlayer() const;
	void DraWHud() const;
	void DrawHudResource(Rectangle rect, Vector2 pos, const std::string &text) const;
	void DrawDiagnostics() const;

	Sign *sign_;
	LevelInfo *level_;
	Player *player_;
	Texture2D state_{};
	Font font20_{};
	Font font14_{};
	Font font_pixel_{};
};
