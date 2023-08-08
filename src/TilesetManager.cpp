#include "TilesetManager.h"

TilesetManager::TilesetMap TilesetManager::Tilesets{};

Tileset *TilesetManager::Get(const char *fileName)
{
	auto it = Tilesets.find(fileName);

	if (it == Tilesets.end())
	{
		auto tileset = new Tileset(fileName);

		Tilesets[fileName] = tileset;

		return tileset;
	}

	return it->second;
}