#include "Tileset.h"

#include <fstream>
#include <cstdint>

static constexpr uint32_t ToLE(uint32_t i)
{
	return ((i >> 24) & 0xFF) | ((i >> 8) & 0xFF00);
}

static void SkipArray(std::ifstream &stream)
{
	uint32_t size;

	stream.read((char *) &size, 4);
	if (!stream)
	{
		return;
	}

	auto arrayLength = ToLE(size);

	stream.seekg(arrayLength * 4, std::ios_base::cur);
}

static void LoadArray(std::ifstream &stream, int *tiles, int tileCount, TileType type)
{
	uint32_t size;

	stream.read((char *) &size, 4);
	if (!stream)
	{
		return;
	}

	auto arrayLength = ToLE(size);
	auto arrayData = new uint32_t[arrayLength];

	stream.read((char *) arrayData, arrayLength * 4);

	for (auto i = 0; i < arrayLength; ++i)
	{
		auto tileData = arrayData[i];
		auto tile = ToLE(tileData);

		if (tile >= tileCount)
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

	delete[] arrayData;
}

int *LoadArrays(int tileCount)
{
	std::ifstream stream("arrays.dat", std::ios::binary);

	auto tiles = new int[tileCount];
	for (auto i = 0; i < tileCount; ++i)
	{
		tiles[i] = TileType::Wall;
	}

	LoadArray(stream, tiles, tileCount, TileType::Passable);
	LoadArray(stream, tiles, tileCount, TileType::Water);
	SkipArray(stream);
	SkipArray(stream);
	LoadArray(stream, tiles, tileCount, TileType::Chair);
	LoadArray(stream, tiles, tileCount, TileType::BedTop);
	LoadArray(stream, tiles, tileCount, TileType::BedBottom);
	LoadArray(stream, tiles, tileCount, TileType::WaterShallow);
	LoadArray(stream, tiles, tileCount, TileType::Jump);
	LoadArray(stream, tiles, tileCount, TileType::Swamp);

	stream.close();

	return tiles;
}

Tileset::Tileset(const char *fileName) : _texture(TextureManager::Get(fileName))
{
	_tileWidth = 16.0f / (float) _texture.width;
	_tileHeight = 16.0f / (float) _texture.height;

	auto tilesX = _texture.width / 16;
	auto tilesY = _texture.height / 16;

	_tileCount = tilesX * tilesY;
	_tiles = (TileType *) LoadArrays(_tileCount);
}

TileType Tileset::GetType(int tileId) const
{
	if (_tiles == nullptr || tileId < 0 || tileId >= _tileCount)
	{
		return TileType::Passable;
	}

	return _tiles[tileId];
}