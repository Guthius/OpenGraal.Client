#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "Animation.h"

class AnimationManager
{
public:
	static auto Get(const std::string &name) -> Animation *;
	static void LoadFrom(const std::filesystem::path &path);

private:
	static void Load(const std::filesystem::path &path);

	static std::map<std::string, Animation *> Animations;
};
