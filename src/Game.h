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
	Game() : _player()
	{
		_level = new LevelInfo(
				Level::Load("levels/onlinestartlocal.nw"),
				TilesetManager::Get("pics1.png"),
				0, 0);
	}

public:
	void Run();

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