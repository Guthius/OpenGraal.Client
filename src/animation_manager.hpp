#pragma once

#include "animation.hpp"

#include <filesystem>
#include <string>

auto load_animation(const std::string &name) -> animation *;
void load_animations_from_directory(const std::filesystem::path &path);
