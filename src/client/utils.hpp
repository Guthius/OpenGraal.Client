#pragma once

#include <raylib.h>
#include <vector>

auto find_sprite_rects(const Texture2D &texture) -> std::vector<Rectangle>;
