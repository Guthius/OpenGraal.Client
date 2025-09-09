#include "Tileset.h"

#include <cstdint>
#include <fstream>

#include "TextureManager.h"

static constexpr auto ToLE(const uint32_t i) -> uint32_t
{
	return i >> 24 & 0xFF | i >> 8 & 0xFF00;
}

static void SkipArray(std::ifstream &stream)
{
	uint32_t size;

	stream.read(reinterpret_cast<char *>(&size), 4);
	if (!stream)
	{
		return;
	}

	const auto array_length = ToLE(size);

	stream.seekg(array_length * 4, std::ios_base::cur);
}

static void LoadArray(std::ifstream &stream, int *tiles, const int tile_count, const int type)
{
	uint32_t size;

	stream.read(reinterpret_cast<char *>(&size), 4);
	if (!stream)
	{
		return;
	}

	const auto array_length = ToLE(size);
	const auto array_data = new uint32_t[array_length];

	stream.read(reinterpret_cast<char *>(array_data), array_length * 4);

	for (auto i = 0; i < array_length; ++i)
	{
		const auto tile_data = array_data[i];
		const auto tile = ToLE(tile_data);

		if (tile >= tile_count)
		{
			continue;
		}

		if (type == TileType::Passable)
		{
			tiles[tile] = type;
		}
		else
		{
			tiles[tile] |= type;
		}
	}

	delete[] array_data;
}

auto LoadArrays(const int tile_count) -> int *
{
	std::ifstream stream("arrays.dat", std::ios::binary);

	const auto tiles = new int[tile_count];
	for (auto i = 0; i < tile_count; ++i)
	{
		tiles[i] = TileType::Wall;
	}

	LoadArray(stream, tiles, tile_count, TileType::Passable);
	LoadArray(stream, tiles, tile_count, TileType::Water);
	SkipArray(stream);
	SkipArray(stream);
	LoadArray(stream, tiles, tile_count, TileType::Chair);
	LoadArray(stream, tiles, tile_count, TileType::BedTop);
	LoadArray(stream, tiles, tile_count, TileType::BedBottom);
	LoadArray(stream, tiles, tile_count, TileType::WaterShallow);
	LoadArray(stream, tiles, tile_count, TileType::Jump);
	LoadArray(stream, tiles, tile_count, TileType::Swamp);

	stream.close();

	return tiles;
}

Tileset::Tileset(const char *filename) : texture_(TextureManager::Get(filename))
{
	tile_width_ = 16.0f / static_cast<float>(texture_.width);
	tile_height_ = 16.0f / static_cast<float>(texture_.height);

	const auto tiles_x = texture_.width / 16;
	const auto tiles_y = texture_.height / 16;

	tile_count_ = tiles_x * tiles_y;
	tiles_ = LoadArrays(tile_count_);
}

auto Tileset::GetType(const int tile_id) const -> int
{
	if (tiles_ == nullptr || tile_id < 0 || tile_id >= tile_count_)
	{
		return TileType::Passable;
	}

	return tiles_[tile_id];
}
