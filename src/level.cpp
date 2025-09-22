#include "level.hpp"

#include <cmath>
#include <rlgl.h>
#include <boost/algorithm/string.hpp>

#include "utils.hpp"

#define COLOR_LINK1     CLITERAL(Color){ 255, 255, 0, 255 }
#define COLOR_LINK2     CLITERAL(Color){ 255, 128, 0, 255 }

#define COLOR_SIGN1     CLITERAL(Color){ 255, 0, 0, 255 }
#define COLOR_SIGN2     CLITERAL(Color){ 192, 0, 0, 255 }
#define COLOR_SIGN3     CLITERAL(Color){ 128, 0, 0, 255 }

struct LevelObjectPattern
{
	LevelObjectType Type;
	std::array<short, 4> Tiles;
	std::array<short, 4> Replacement;
	LeapType LeapType;
};

static const std::vector<LevelObjectPattern> object_patterns = {
	{
		.Type = LevelObjectType::Bush,
		.Tiles = {2, 3, 18, 19},
		.Replacement = {677, 678, 693, 694},
		.LeapType = LeapType::Leaves
	},
	{
		.Type = LevelObjectType::Grass,
		.Tiles = {420, 421, 436, 437},
		.Replacement = {679, 680, 695, 696},
		.LeapType = LeapType::Grass
	},
	{
		.Type = LevelObjectType::Vase,
		.Tiles = {684, 685, 700, 701},
		.Replacement = {1770, 1771, 1786, 1787}
	},
	{
		.Type = LevelObjectType::Sign,
		.Tiles = {512, 513, 528, 529},
		.Replacement = {1802, 1803, 1818, 1819}
	},
};

LevelLink::LevelLink(const std::string &data)
{
	std::vector<std::string> tokens;

	boost::split(tokens, data, boost::is_any_of(" "));
	if (tokens.size() < 7)
	{
		return;
	}

	const auto offset = tokens.size() - 7;

	auto new_level = tokens[0];
	if (offset > 0)
	{
		for (int i = 0; i < offset; ++i)
		{
			new_level += " " + tokens[offset];
		}
	}

	const auto x = static_cast<float>(static_cast<int>(std::stof(tokens[offset + 1]) * 16));
	const auto y = static_cast<float>(static_cast<int>(std::stof(tokens[offset + 2]) * 16));
	const auto w = static_cast<float>(static_cast<int>(std::stof(tokens[offset + 3]) * 16));
	const auto h = static_cast<float>(static_cast<int>(std::stof(tokens[offset + 4]) * 16));

	rect_ = {x, y, w, h};
	new_level_ = new_level;
	new_x_ = tokens[offset + 5];
	new_y_ = tokens[offset + 6];
}

void level::Draw(const tileset *tileset) const
{
	const auto tileWidth = tileset->GetTileWidth();
	const auto tileHeight = tileset->GetTileHeight();

	rlBegin(RL_QUADS);
	rlColor4ub(255, 255, 255, 255);

	for (int yy = 0; yy < 64; ++yy)
	{
		for (int xx = 0; xx < 64; ++xx)
		{
			const auto sx1 = static_cast<float>(xx * 16); // Left
			const auto sx2 = sx1 + 16; // Right
			const auto sy1 = static_cast<float>(yy * 16); // Top
			const auto sy2 = sy1 + 16; // Bottom

			const auto tileIndex = (yy * 64) + xx;
			const auto tileId = _board[tileIndex];
			const auto tilex = ((tileId / 512) * 16 + (tileId % 16)) * 16;
			const auto tiley = ((tileId % 512) / 16) * 16;

			const auto tx1 = static_cast<float>(tilex) / 2048.0f; // Left
			const auto tx2 = tx1 + tileWidth; // Right
			const auto ty1 = static_cast<float>(tiley) / 512.0f; // Top
			const auto ty2 = ty1 + tileHeight; // Bottom

			rlColor4ub(255, 255, 255, 255);

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

	// DrawEditorHints();

	rlEnd();
}

void level::DrawEditorHints() const
{
	for (const auto &link: _links)
	{
		const auto [x, y, w, h] = link.GetRectangle();

		DrawRectangleLines(x, y, w, h, COLOR_LINK1);
		DrawRectangleLines(x + 1, y + 1, w - 2, h - 2, COLOR_LINK2);
	}

	for (const auto &sign: _signs)
	{
		const auto [x, y, w, h] = sign.GetRectangle();

		DrawRectangleLines(x, y, w, h, COLOR_SIGN1);
		DrawRectangleLines(x + 1, y + 1, w - 2, h - 2, COLOR_SIGN2);
		DrawRectangleLines(x + 2, y + 2, w - 4, h - 4, COLOR_SIGN3);
	}
}

auto level::GetLinkAt(const int x, const int y) const -> const LevelLink *
{
	for (const auto &link: _links)
	{
		auto &rect = link.GetRectangle();

		if (x >= rect.x && x <= rect.x + rect.width &&
		    y >= rect.y && y <= rect.y + rect.height)
		{
			return &link;
		}
	}

	return nullptr;
}

auto level::GetSignAt(const int x, const int y) const -> const level_sign *
{
	for (const auto &sign: _signs)
	{
		auto &rect = sign.GetRectangle();

		if (x >= rect.x && x <= rect.x + rect.width &&
		    y >= rect.y && y <= rect.y + rect.height)
		{
			return &sign;
		}
	}

	return nullptr;
}

auto level::GetTileType(const tileset *tileset, const int x, const int y) const -> int
{
	const auto tx = x / 16;
	const auto ty = y / 16;

	if (tx < 0 || tx > 63 || ty < 0 || ty > 63)
	{
		return TileType::Passable;
	}

	const auto tileIndex = ty * 64 + tx;
	const auto tileId = _board[tileIndex];

	return tileset->GetType(tileId);
}

auto level::GetTileId(const int x, const int y) const -> int
{
	const auto tx = x / 16;
	const auto ty = y / 16;

	if (tx < 0 || tx > 63 || ty < 0 || ty > 63)
	{
		return -1;
	}

	const auto tileIndex = ty * 64 + tx;
	return _board[tileIndex];
}

auto level::OnWall(const tileset *tileset, const Rectangle rect) const -> bool
{
	if (constexpr float map_size = 64.0f * 16.0f;
		rect.x >= map_size ||
		rect.y >= map_size ||
		rect.x + rect.width <= 0.0f ||
		rect.y + rect.height <= 0.0f)
	{
		return true;
	}

	auto sx = static_cast<int>(std::floor(rect.x / 16.0f));
	auto sy = static_cast<int>(std::floor(rect.y / 16.0f));
	auto dx = static_cast<int>(std::floor((rect.x + rect.width - 0.001f) / 16.0f));
	auto dy = static_cast<int>(std::floor((rect.y + rect.height - 0.001f) / 16.0f));

	sx = std::max(0, std::min(63, sx));
	sy = std::max(0, std::min(63, sy));
	dx = std::max(0, std::min(63, dx));
	dy = std::max(0, std::min(63, dy));

	for (int y = sy; y <= dy; ++y)
	{
		for (int x = sx; x <= dx; ++x)
		{
			const int tile_index = y * 64 + x;

			if (const auto tile_id = _board[tile_index]; tileset->GetType(tile_id) & TileType::Wall)
			{
				return true;
			}
		}
	}

	return false;
}

auto level::OnWall(const tileset *tileset, const Vector2 pt) const -> bool
{
	if (constexpr float map_size = 64.0f * 16.0f;
		pt.x < 0.0f || pt.y < 0.0f ||
		pt.x >= map_size || pt.y >= map_size)
	{
		return true;
	}

	const auto x = static_cast<int>(pt.x / 16.0f);
	const auto y = static_cast<int>(pt.y / 16.0f);

	const auto tile_index = y * 64 + x;
	const auto tile_id = _board[tile_index];

	return (tileset->GetType(tile_id) & TileType::Wall) != 0;
}

auto level::MatchObjectAt(const int x, const int y) const -> LevelObjectMatch
{
	constexpr auto in_bounds = [&](const int dx, const int dy) -> bool {
		return dx >= 0 && dx < 64 && dy >= 0 && dy < 64;
	};

	const auto match_pattern_at = [&](const int dx, const int dy, const std::array<short, 4> &tile_ids) -> bool {
		for (auto yy = 0; yy < 2; yy++)
		{
			for (auto xx = 0; xx < 2; xx++)
			{
				const auto tx = dx + xx;
				const auto ty = dy + yy;

				if (!in_bounds(tx, ty))
				{
					return false;
				}

				if (tile_ids[yy * 2 + xx] != _board[ty * 64 + tx])
				{
					return false;
				}
			}
		}

		return true;
	};

	const auto tx = x / 16;
	const auto ty = y / 16;

	for (const auto &[object_type, tile_ids, replacement_tile_ids, leap_type]: object_patterns)
	{
		for (auto dy = ty - 1; dy <= ty; dy++)
		{
			for (auto dx = tx - 1; dx <= tx; dx++)
			{
				if (match_pattern_at(dx, dy, tile_ids))
				{
					return {
						.Type = object_type,
						.X = dx, .Y = dy,
						.Replacement = {
							replacement_tile_ids[0],
							replacement_tile_ids[1],
							replacement_tile_ids[2],
							replacement_tile_ids[3]
						},
						.LeapType = leap_type
					};
				}
			}
		}
	}

	return {LevelObjectType::None};
}

auto level::DestroyObjectAt(const int x, const int y) -> std::tuple<LeapType, int, int>
{
	const auto is_destructible = [&](const LevelObjectType type) -> bool {
		return type == LevelObjectType::Grass ||
		       type == LevelObjectType::Bush;
	};

	const auto replace_tiles_at = [&](const int dx, const int dy, const std::array<short, 4> &tile_ids) {
		for (auto yy = 0; yy < 2; yy++)
		{
			for (auto xx = 0; xx < 2; xx++)
			{
				const auto tx = dx + xx;
				const auto ty = dy + yy;

				_board[ty * 64 + tx] = tile_ids[yy * 2 + xx];
			}
		}
	};

	const auto [object_type, dx, dy, replacement_tile_ids, leap_type] = MatchObjectAt(x, y);
	if (!is_destructible(object_type))
	{
		return {LeapType::None, 0, 0};
	}

	replace_tiles_at(dx, dy, replacement_tile_ids);

	return {leap_type, dx * 16, dy * 16};
}

auto level::LiftObjectAt(const int x, const int y) -> actor::CarriedItem
{
	const auto get_carried_item_type = [&](const LevelObjectType type) -> actor::CarriedItem {
		switch (type)
		{
			case LevelObjectType::Bush: return actor::CarriedItem::Bush;
			case LevelObjectType::Vase: return actor::CarriedItem::Vase;
			case LevelObjectType::Sign: return actor::CarriedItem::Sign;
			default:
				return actor::CarriedItem::None;
		}
	};

	const auto replace_tiles_at = [&](const int dx, const int dy, const std::array<short, 4> &tile_ids) {
		for (auto yy = 0; yy < 2; yy++)
		{
			for (auto xx = 0; xx < 2; xx++)
			{
				const auto tx = dx + xx;
				const auto ty = dy + yy;

				_board[ty * 64 + tx] = tile_ids[yy * 2 + xx];
			}
		}
	};

	const auto [object_type, dx, dy, replacement_tile_ids, leap_type] = MatchObjectAt(x, y);

	const auto carried_item_type = get_carried_item_type(object_type);
	if (carried_item_type == actor::CarriedItem::None)
	{
		return actor::CarriedItem::None;
	}

	replace_tiles_at(dx, dy, replacement_tile_ids);

	return carried_item_type;
}

auto level::Load(const std::filesystem::path &path) -> level *
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

auto level::LoadNw(std::ifstream &stream) -> level *
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

		const auto x = std::stoi(tokens[1]);
		const auto y = std::stoi(tokens[2]);
		const auto w = std::stoi(tokens[3]);
		const auto z = std::stoi(tokens[4]);

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
			const auto j = i * 2;

			const auto b1 = base64.find_first_of(data[j]) << 6;
			const auto b2 = base64.find_first_of(data[j + 1]);

			const auto tileId = b1 | b2;
			const auto tileIndex = (y * 64) + (x + i);

			board[tileIndex] = static_cast<short>(tileId);
		}
	}

	constexpr std::vector<LevelLink> links;
	constexpr std::vector<level_sign> signs;

	return new level(board, links, signs);
}

auto level::LoadGraal(std::ifstream &stream, const int bits, const size_t code_mask, const size_t control_bit, const bool has_chests) -> level *
{
	constexpr int boardSize = 64 * 64;

	int bitsRead = 0;
	char byte;
	size_t buf = 0;
	short tile1 = -1;
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

		const uint16_t code = buf & code_mask;
		buf >>= bits;
		bitsRead -= bits;

		if (code & control_bit)
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

			const auto tile2 = static_cast<short>(code);

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
	std::vector<level_sign> signs;
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

	char baddyX;
	char baddyY;
	char baddyType;

	/* Read Baddies */
	while (!stream.eof())
	{
		stream.read(&baddyX, 1);
		stream.read(&baddyY, 1);
		stream.read(&baddyType, 1);

		std::getline(stream, line);

		if (!stream)
		{
			break;
		}

		if (baddyX == -1 && baddyY == -1 && baddyType == -1)
		{
			break;
		}
	}

	/* Read NPC's */
	while (std::getline(stream, line))
	{
		boost::trim(line);
		if (line.empty() || line == "#")
		{
			break;
		}
	}

	/* Read Chests */
	if (has_chests)
	{
		while (std::getline(stream, line))
		{
			boost::trim(line);
			if (line.empty() || line == "#")
			{
				break;
			}
		}
	}

	/* Read Signs */
	while (std::getline(stream, line))
	{
		if (line.empty())
		{
			break;
		}

		const auto x = static_cast<float>(line[0] - 32);
		const auto y = static_cast<float>(line[1] - 32);
		auto text = level_sign::Decode(line.substr(2));

		signs.emplace_back(x * 16, y * 16, text);
	}

	return new level(board, links, signs);
}

auto level::LoadGraal(std::ifstream &stream, const char *version) -> level *
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
		v > 0 ? 0x1000 : 0x800,
		v > 0);
}
