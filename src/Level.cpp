#include "Level.h"
#include "Utils.h"

#include <rlgl.h>

void Level::Draw(Tileset *tileset) const
{
	auto tileWidth = tileset->GetTileWidth();
	auto tileHeight = tileset->GetTileHeight();

	rlBegin(RL_QUADS);
	rlColor4ub(255, 255, 255, 255);

	for (int yy = 0; yy < 64; ++yy)
	{
		for (int xx = 0; xx < 64; ++xx)
		{
			auto sx1 = static_cast<float>(xx * 16); // Left
			auto sx2 = sx1 + 16; // Right
			auto sy1 = static_cast<float>(yy * 16); // Top
			auto sy2 = sy1 + 16; // Bottom

			auto tileIndex = (yy * 64) + xx;
			auto tileId = _board[tileIndex];
			auto tilex = ((tileId / 512) * 16 + (tileId % 16)) * 16;
			auto tiley = ((tileId % 512) / 16) * 16;

			auto tx1 = static_cast<float>(tilex) / 2048.0f; // Left
			auto tx2 = tx1 + tileWidth; // Right
			auto ty1 = static_cast<float>(tiley) / 512.0f; // Top
			auto ty2 = ty1 + tileHeight; // Bottom

			rlTexCoord2f(tx1, ty1);
			rlVertex2f(sx1, sy1);

			rlTexCoord2f(tx1, ty2);
			rlVertex2f(sx1, sy2);

			rlTexCoord2f(tx2, ty2);
			rlVertex2f(sx2, sy2);

			rlTexCoord2f(tx2, ty1);
			rlVertex2f(sx2, sy1);
		}
	}

	rlEnd();
}

Level *Level::Load(const std::filesystem::path &path)
{
	if (!is_regular_file(path))
	{
		return nullptr;
	}

	auto file = std::ifstream(path, std::ios::binary);
	if (!file)
	{
		return nullptr;
	}

	std::string line;
	if (!std::getline(file, line) || line.size() < 8 || line.substr(0, 8) != "GLEVNW01")
	{
		return nullptr;
	}

	return LoadNw(file);
}

Level *Level::LoadNw(std::ifstream &stream)
{
	static std::string base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

	std::vector<short> board(64 * 64);

	std::string line;
	while (std::getline(stream, line))
	{
		auto tokens = Split(line);

		if (tokens.empty() || tokens[0] != "BOARD")
		{
			continue;
		}

		if (tokens.size() != 6)
		{
			continue;
		}

		auto x = std::stoi(tokens[1]);
		auto y = std::stoi(tokens[2]);
		auto w = std::stoi(tokens[3]);
		auto z = std::stoi(tokens[4]);

		if (x < 0 || x >= 64 || y < 0 || y >= 64)
		{
			continue;
		}

		if (z != 0)
		{
			continue;
		}

		auto &data = tokens[5];
		if (data.size() < w * 2)
		{
			continue;
		}

		for (int i = 0; i < w; ++i)
		{
			auto j = i * 2;

			auto b1 = base64.find_first_of(data[j]) << 6;
			auto b2 = base64.find_first_of(data[j + 1]);

			auto tileId = b1 | b2;
			auto tileIndex = (y * 64) + (x + i);

			board[tileIndex] = static_cast<short>(tileId);
		}
	}

	return new Level(board);
}