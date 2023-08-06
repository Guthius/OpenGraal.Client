#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

#include "Tileset.h"

class Level
{
public:
	explicit Level(const std::vector<short> &board) : _board(board)
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
};