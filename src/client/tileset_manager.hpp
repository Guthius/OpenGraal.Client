#pragma once

#include "tileset.hpp"

auto load_tileset(const std::string &texture_filename, int format = og::shared::tileset_format_pics1) -> tileset *;
void add_tile_def(const std::string &texture_filename, const std::string &level_prefix, int format);
void clear_tile_defs();

[[nodiscard]]
auto tileset_for_level(const std::string &level_name) -> tileset *;
