#pragma once

#include <array>
#include <filesystem>
#include <vector>

#include "carry_object.hpp"
#include "leap_effect.hpp"
#include "level_link.hpp"
#include "level_sign.hpp"
#include "tileset.hpp"

enum class tile_pattern_type
{
	none,
	bush,
	swamp,
	vase,
	sign
};

struct tile_pattern_match
{
	tile_pattern_type type;
	int x, y;
	std::array<short, 4> replacement;
	leap_effect_type leap_type;
};

class level
{
public:
	explicit level(
		const std::vector<short> &board,
		const std::vector<level_link> &links,
		const std::vector<level_sign> &signs)
		: board_(board), links_(links), signs_(signs)
	{
	}

	void draw(const tileset *tileset) const;

	[[nodiscard]] auto get_link_at(float x, float y) const -> const level_link *;
	[[nodiscard]] auto get_sign_at(float x, float y) const -> const level_sign *;
	[[nodiscard]] auto get_tile_type(const tileset *tileset, int x, int y) const -> int;
	[[nodiscard]] auto get_tile_id(int x, int y) const -> int;

	[[nodiscard]] auto on_wall(const tileset *tileset, Rectangle rect) const -> bool;
	[[nodiscard]] auto on_wall(const tileset *tileset, Vector2 pt) const -> bool;

	[[nodiscard]] auto find_tile_pattern_at(float x, float y) const -> tile_pattern_match;

	auto try_destroy_object_at(float x, float y) -> std::tuple<leap_effect_type, int, int>;
	auto try_lift_object_at(float x, float y) -> carry_object_type;

	static auto load(const std::filesystem::path &path) -> std::shared_ptr<level>;

private:
	static auto load_nw(std::ifstream &stream) -> std::shared_ptr<level>;
	static auto load_graal(std::ifstream &stream, int bits, size_t code_mask, size_t control_bit, bool has_chests) -> std::shared_ptr<level>;
	static auto load_graal(std::ifstream &stream, const char *version) -> std::shared_ptr<level>;

	std::vector<short> board_;
	std::vector<level_link> links_{};
	std::vector<level_sign> signs_;
};
