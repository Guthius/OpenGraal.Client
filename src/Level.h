#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

#include "Tileset.h"
#include "LevelSign.h"

class LevelLink
{
public:
	explicit LevelLink(const std::string &data);

public:
	[[nodiscard]] const std::string &GetNewLevel() const { return _newLevel; }

	[[nodiscard]] const std::string &GetNewX() const { return _newX; }

	[[nodiscard]] const std::string &GetNewY() const { return _newY; }

	[[nodiscard]] const Rectangle &GetRectangle() const { return _rect; }

private:
	std::string _newLevel{};
	std::string _newX{};
	std::string _newY{};
	Rectangle _rect{};
};

class Level
{
public:
	explicit Level(
			const std::vector<short> &board,
			const std::vector<LevelLink> &links,
			const std::vector<LevelSign> &signs)
			: _board(board), _links(links), _signs(signs)
	{
	}

	void Draw(Tileset *tileset) const;

	void DrawEditorHints() const;

	[[nodiscard]] const LevelLink *GetLinkAt(int x, int y) const;

	[[nodiscard]] const LevelSign *GetSignAt(int x, int y) const;

	[[nodiscard]] TileType GetTileType(Tileset *tileset, int x, int y) const;

	[[nodiscard]] bool OnWall(Tileset *tileset, Rectangle rect) const;

public:
	static Level *Load(const std::filesystem::path &path);

private:
	static Level *LoadNw(std::ifstream &stream);

	static Level *LoadGraal(std::ifstream &stream, int bits, size_t codeMask, size_t controlBit, bool hasChests);

	static Level *LoadGraal(std::ifstream &stream, const char *version);

private:
	std::vector<short> _board;
	std::vector<LevelLink> _links;
	std::vector<LevelSign> _signs;
};