#pragma once

#include <raylib.h>

class Tileset
{
public:
	explicit Tileset(const char *fileName) : _texture(LoadTexture(fileName))
	{
		_tileWidth = 16.0f / (float) _texture.width;
		_tileHeight = 16.0f / (float) _texture.height;
	}

public:
	[[nodiscard]] Texture2D GetTexture() const { return _texture; }

	[[nodiscard]] float GetTileWidth() const { return _tileWidth; }

	[[nodiscard]] float GetTileHeight() const { return _tileHeight; }

private:
	static Texture2D LoadTexture(const char *fileName)
	{
		auto image = LoadImage(fileName);

		return LoadTextureFromImage(image);
	}

private:
	Texture2D _texture;
	float _tileWidth;
	float _tileHeight;
};

class TilesetManager {
public:
	static Tileset *Get(const char *fileName);
};