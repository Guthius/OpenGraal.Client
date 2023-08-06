#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

#include "Tileset.h"

class LevelLink
{
public:
	explicit LevelLink(const std::string &data);

public:
	[[nodiscard]] const std::string &GetNewLevel() const { return _newLevel; }

	[[nodiscard]] const std::string &GetNewX() const { return _newX; }

	[[nodiscard]] const std::string &GetNewY() const { return _newY; }

	[[nodiscard]] int GetX() const { return _x; }

	[[nodiscard]] int GetY() const { return _y; }

	[[nodiscard]] int GetWidth() const { return _w; }

	[[nodiscard]] int GetHeight() const { return _h; }

private:
	std::string _newLevel{};
	std::string _newX{};
	std::string _newY{};
	int _x = 0;
	int _y = 0;
	int _w = 0;
	int _h = 0;
};

class Level
{
public:
	explicit Level(const std::vector<short> &board, const std::vector<LevelLink> &links)
			: _board(board), _links(links)
	{
	}

	void Draw(Tileset *tileset) const;

public:
	static Level *Load(const std::filesystem::path &path);

private:
	static Level *LoadNw(std::ifstream &stream);

	static Level *LoadGraal(std::ifstream &stream, int bits, size_t codeMask, size_t controlBit);

	static Level *LoadGraal(std::ifstream &stream, const char *version);

private:
	std::vector<short> _board;
	std::vector<LevelLink> _links;
};