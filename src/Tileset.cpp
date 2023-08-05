#include "Tileset.h"

#include <string>
#include <map>

std::map<std::string, Tileset*> tilesets{};

Tileset *TilesetManager::Get(const char *fileName)
{
	auto it = tilesets.find(fileName);

	if (it == tilesets.end())
	{
		auto tileset = new Tileset(fileName);

		tilesets[fileName] = tileset;

		return tileset;
	}

	return it->second;
}