#pragma once

#include <raylib.h>

struct TileType
{
	enum
	{
		Passable = 0,
		Wall = 1 << 0,
		Water = 1 << 1,
		Chair = 1 << 2,
		WaterShallow = 1 << 3,
		Swamp = 1 << 4,
		Jump = 1 << 5,
		BedTop = 1 << 6,
		BedBottom = 1 << 7,

		Unknown = 1 << 16
	};
};

class Tileset
{
public:
	explicit Tileset(const char *filename);

	~Tileset()
	{
		if (tiles_ == nullptr)
		{
			return;
		}

		delete[] tiles_;

		tiles_ = nullptr;
	}

	[[nodiscard]] auto GetTexture() const -> Texture2D { return texture_; }
	[[nodiscard]] auto GetTileWidth() const -> float { return tile_width_; }
	[[nodiscard]] auto GetTileHeight() const -> float { return tile_height_; }
	[[nodiscard]] auto GetType(int tile_id) const -> int;

private:
	int tile_count_;
	int *tiles_;
	Texture2D texture_;
	float tile_width_;
	float tile_height_;
};
