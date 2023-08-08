#pragma once

#include "Tileset.h"
#include <map>
#include <string>

class TilesetManager
{
private:
	typedef std::map<std::string, Tileset*> TilesetMap;

public:
	static Tileset *Get(const char *fileName);

private:
	static TilesetMap Tilesets;
};