#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

#include "LevelSign.h"
#include "Tileset.h"

class LevelLink
{
public:
	explicit LevelLink(const std::string &data);

	[[nodiscard]] auto GetNewLevel() const -> const std::string & { return new_level_; }
	[[nodiscard]] auto GetNewX() const -> const std::string & { return new_x_; }
	[[nodiscard]] auto GetNewY() const -> const std::string & { return new_y_; }
	[[nodiscard]] auto GetRectangle() const -> const Rectangle & { return rect_; }

private:
	std::string new_level_{};
	std::string new_x_{};
	std::string new_y_{};
	Rectangle rect_{};
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

	void Draw(const Tileset *tileset) const;
	void DrawEditorHints() const;

	[[nodiscard]] auto GetLinkAt(int x, int y) const -> const LevelLink *;
	[[nodiscard]] auto GetSignAt(int x, int y) const -> const LevelSign *;
	[[nodiscard]] auto GetTileType(const Tileset *tileset, int x, int y) const -> int;
	[[nodiscard]] auto OnWall(const Tileset *tileset, Rectangle rect) const -> bool;
	[[nodiscard]] auto OnWall(const Tileset *tileset, Vector2 pt) const -> bool;

	static auto Load(const std::filesystem::path &path) -> Level *;

private:
	static auto LoadNw(std::ifstream &stream) -> Level *;
	static auto LoadGraal(std::ifstream &stream, int bits, size_t code_mask, size_t control_bit, bool has_chests) -> Level *;
	static auto LoadGraal(std::ifstream &stream, const char *version) -> Level *;

	std::vector<short> _board;
	std::vector<LevelLink> _links;
	std::vector<LevelSign> _signs;
};
