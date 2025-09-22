#pragma once

#include "tileset.hpp"
#include <map>
#include <string>

class tileset_manager
{
	using TilesetMap = std::map<std::string, tileset *>;

public:
	static auto Get(const char *filename) -> tileset *;

private:
	static TilesetMap Tilesets;
};
