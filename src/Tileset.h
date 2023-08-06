#pragma once

#include "TextureManager.h"
#include <raylib.h>

class Tileset
{
public:
	explicit Tileset(const char *fileName) : _texture(TextureManager::Get(fileName))
	{
		_tileWidth = 16.0f / (float) _texture.width;
		_tileHeight = 16.0f / (float) _texture.height;
	}

public:
	[[nodiscard]] Texture2D GetTexture() const { return _texture; }

	[[nodiscard]] float GetTileWidth() const { return _tileWidth; }

	[[nodiscard]] float GetTileHeight() const { return _tileHeight; }

private:
	Texture2D _texture;
	float _tileWidth;
	float _tileHeight;
};

class TilesetManager {
public:
	static Tileset *Get(const char *fileName);
};