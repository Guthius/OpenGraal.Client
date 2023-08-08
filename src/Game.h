#pragma once

#include "Level.h"
#include "Tileset.h"
#include "Actor.h"
#include "Player.h"

class Game
{
private:
	struct LevelInfo
	{
		LevelInfo(Level *level, Tileset *tileset, int x, int y)
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

public:
	void Run();

	void ChangeLevel(const std::string &levelName);

	Level *GetCurrentLevel() const { return _level->Level; }
	bool OnWall(Rectangle rect) const;
	TileType GetTileType(int x, int y) const;

private:
	void Update();

	void Draw() const;

	void DrawPlayer() const;

	void DrawUI() const;
	void DrawUI_Resource(Rectangle rect, Vector2 pos, const std::string& text) const;

private:
	static void DrawDiagnostics();

private:
	LevelInfo *_level;
	Player *_player;
	Texture2D _state;
	Font _font20;
	Font _font14;
};