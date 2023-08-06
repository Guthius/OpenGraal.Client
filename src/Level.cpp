#include "Level.h"
#include "Utils.h"

#include <rlgl.h>
#include <boost/algorithm/string.hpp>

LevelLink::LevelLink(const std::string &data)
{
	std::vector<std::string> tokens;

	boost::split(tokens, data, boost::is_any_of(" "));
	if (tokens.size() < 7)
	{
		return;
	}

	auto offset = tokens.size() - 7;

	auto newLevel = tokens[0];
	if (offset > 0)
	{
		for (int i = 0; i < offset; ++i)
		{
			newLevel += " " + tokens[offset];
		}
	}

	_x = static_cast<int>(std::stof(tokens[offset + 1]) * 16);
	_y = static_cast<int>(std::stof(tokens[offset + 2]) * 16);
	_w = static_cast<int>(std::stof(tokens[offset + 3]) * 16);
	_h = static_cast<int>(std::stof(tokens[offset + 4]) * 16);

	_newX = tokens[offset + 5];
	_newY = tokens[offset + 6];
}

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

	auto stream = std::ifstream(path, std::ios::binary);
	if (!stream)
	{
		return nullptr;
	}

	char version[9]{};

	stream.read(version, 8);
	if (!stream)
	{
		return nullptr;
	}

	if (TextIsEqual(version, "GLEVNW01"))
	{
		return LoadNw(stream);
	}

	return LoadGraal(stream, version);
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

	std::vector<LevelLink> links;

	return new Level(board, links);
}

Level *Level::LoadGraal(std::ifstream &stream, int bits, size_t codeMask, size_t controlBit)
{
	constexpr int boardSize = 64 * 64;

	int bitsRead = 0;
	char byte;
	size_t buf = 0;
	uint16_t code;
	short tile1 = -1;
	short tile2;
	int boardIndex = 0;
	bool doubleMode = false;
	int count = 1;
	std::vector<short> board(64 * 64);

	while (boardIndex < boardSize && !stream.eof())
	{
		while (bitsRead < bits)
		{
			stream.read(&byte, 1);

			buf |= static_cast<uint8_t>(byte) << bitsRead;

			bitsRead += 8;
		}

		code = buf & codeMask;
		buf >>= bits;
		bitsRead -= bits;

		if (code & controlBit)
		{
			doubleMode = (code & 0x100) == 0x100;
			count = code & 0xFF;
			continue;
		}

		if (doubleMode)
		{
			if (tile1 == -1)
			{
				tile1 = static_cast<short>(code);
				continue;
			}

			tile2 = static_cast<short>(code);

			for (auto i = 0; i < count && boardIndex < boardSize - 1; ++i)
			{
				board[boardIndex++] = tile1;
				board[boardIndex++] = tile2;
			}

			tile1 = -1;
			doubleMode = false;
		}
		else
		{
			for (auto i = 0; i < count && boardIndex < boardSize; ++i)
			{
				board[boardIndex++] = static_cast<short>(code);
			}
		}

		count = 1;
	}

	std::vector<LevelLink> links;
	std::string line;

	while (std::getline(stream, line))
	{
		boost::trim(line);
		if (line.empty() || line == "#")
		{
			break;
		}

		links.emplace_back(line);
	}

	return new Level(board, links);
}

Level *Level::LoadGraal(std::ifstream &stream, const char *version)
{
	auto v = -1;

	if (TextIsEqual(version, "GR-V1.00")) v = 0;
	else if (TextIsEqual(version, "GR-V1.01")) v = 1;
	else if (TextIsEqual(version, "GR-V1.02")) v = 2;
	else if (TextIsEqual(version, "GR-V1.03")) v = 3;

	if (v == -1)
	{
		return nullptr;
	}

	return LoadGraal(
			stream,
			v > 0 ? 13 : 12,
			v > 0 ? 0x1FFF : 0xFFF,
			v > 0 ? 0x1000 : 0x800);
}