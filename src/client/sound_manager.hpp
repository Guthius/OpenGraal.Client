#pragma once

#include <raylib.h>
#include <string>

auto load_sound(const std::string &filename) -> Sound;
void play_sound(const std::string &filename);
