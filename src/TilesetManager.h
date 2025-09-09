#pragma once

#include "Tileset.h"
#include <map>
#include <string>

class TilesetManager
{
	using TilesetMap = std::map<std::string, Tileset *>;

public:
	static auto Get(const char *filename) -> Tileset *;

private:
	static TilesetMap Tilesets;
};
