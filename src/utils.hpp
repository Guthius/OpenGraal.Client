#pragma once

#include <raylib.h>
#include <string>
#include <vector>

auto split_string(const std::string &str) -> std::vector<std::string>;

auto find_sprite_rects(const Texture2D &texture) -> std::vector<Rectangle>;
