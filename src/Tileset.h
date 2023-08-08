#pragma once

#include "TextureManager.h"
#include <raylib.h>

enum TileType
{
	Passable = 0,
	Wall = (1 << 0),
	Water = (1 << 1),
	Chair = (1 << 2),
	WaterShallow = (1 << 3),
	Swamp = (1 << 4),
	Jump = (1 << 5),
	BedTop = (1 << 6),
	BedBottom = (1 << 7),

	Unknown = (1 << 16),
};

class Tileset
{
public:
	explicit Tileset(const char *fileName);

	~Tileset()
	{
		if (_tiles == nullptr)
		{
			return;
		}

		delete[] _tiles;

		_tiles = nullptr;
	}

public:
	[[nodiscard]] Texture2D GetTexture() const { return _texture; }

	[[nodiscard]] float GetTileWidth() const { return _tileWidth; }

	[[nodiscard]] float GetTileHeight() const { return _tileHeight; }

	[[nodiscard]] TileType GetType(int tileId) const;

private:
	int _tileCount;
	TileType *_tiles;
	Texture2D _texture;
	float _tileWidth;
	float _tileHeight;
};