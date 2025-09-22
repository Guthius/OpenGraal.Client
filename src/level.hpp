#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <vector>

#include "leaps.hpp"
#include "level_sign.hpp"
#include "tileset.hpp"

enum class LevelObjectType
{
	None,
	Bush,
	Grass,
	Vase,
	Sign
};

struct LevelObjectMatch
{
	LevelObjectType Type;
	int X, Y;
	std::array<short, 4> Replacement;
	LeapType LeapType;
};

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

class level
{
public:
	explicit level(
		const std::vector<short> &board,
		const std::vector<LevelLink> &links,
		const std::vector<level_sign> &signs)
		: _board(board), _links(links), _signs(signs)
	{
	}

	void Draw(const tileset *tileset) const;
	void DrawEditorHints() const;

	[[nodiscard]] auto GetLinkAt(int x, int y) const -> const LevelLink *;
	[[nodiscard]] auto GetSignAt(int x, int y) const -> const level_sign *;
	[[nodiscard]] auto GetTileType(const tileset *tileset, int x, int y) const -> int;
	[[nodiscard]] auto GetTileId(int x, int y) const -> int;
	[[nodiscard]] auto OnWall(const tileset *tileset, Rectangle rect) const -> bool;
	[[nodiscard]] auto OnWall(const tileset *tileset, Vector2 pt) const -> bool;
	[[nodiscard]] auto MatchObjectAt(int x, int y) const -> LevelObjectMatch;

	auto DestroyObjectAt(int x, int y) -> std::tuple<LeapType, int, int>;
	auto LiftObjectAt(int x, int y) -> actor::CarriedItem;

	static auto Load(const std::filesystem::path &path) -> level *;

private:
	static auto LoadNw(std::ifstream &stream) -> level *;
	static auto LoadGraal(std::ifstream &stream, int bits, size_t code_mask, size_t control_bit, bool has_chests) -> level *;
	static auto LoadGraal(std::ifstream &stream, const char *version) -> level *;

	std::vector<short> _board;
	std::vector<LevelLink> _links;
	std::vector<level_sign> _signs;
};
