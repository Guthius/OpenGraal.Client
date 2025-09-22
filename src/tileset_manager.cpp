#include "tileset_manager.hpp"

tileset_manager::TilesetMap tileset_manager::Tilesets{};

auto tileset_manager::Get(const char *filename) -> tileset *
{
	const auto it = Tilesets.find(filename);

	if (it == Tilesets.end())
	{
		const auto tileset = new ::tileset(filename);

		Tilesets[filename] = tileset;

		return tileset;
	}

	return it->second;
}
