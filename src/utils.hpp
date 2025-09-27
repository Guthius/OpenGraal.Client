#pragma once

#include <vector>
#include <string>
#include <raylib.h>

auto split_string(const std::string &str) -> std::vector<std::string>;

auto find_sprite_rects(const Texture2D &texture) -> std::vector<Rectangle>;
