#include "TilesetManager.h"

TilesetManager::TilesetMap TilesetManager::Tilesets{};

auto TilesetManager::Get(const char *filename) -> Tileset *
{
	const auto it = Tilesets.find(filename);

	if (it == Tilesets.end())
	{
		const auto tileset = new Tileset(filename);

		Tilesets[filename] = tileset;

		return tileset;
	}

	return it->second;
}
