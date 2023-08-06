#pragma once

#include "Level.h"
#include "Tileset.h"
#include "Actors/Actor.h"
#include "Actors/Player.h"

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

private:
	void Update();
	void Draw() const;
	void DrawPlayer() const;

private:
	static void DrawDiagnostics();

private:
	LevelInfo *_level;
	Player _player;
};