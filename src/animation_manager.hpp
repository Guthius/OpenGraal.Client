#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "animation.hpp"

class animation_manager
{
public:
	static auto Get(const std::string &name) -> animation *;
	static void LoadFrom(const std::filesystem::path &path);

private:
	static void Load(const std::filesystem::path &path);

	static std::map<std::string, animation *> Animations;
};
