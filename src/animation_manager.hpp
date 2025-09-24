#pragma once

#include <filesystem>
#include <string>

#include "animation.hpp"

auto load_animation(const std::string &name) -> animation *;
void load_animations_from_directory(const std::filesystem::path &path);
