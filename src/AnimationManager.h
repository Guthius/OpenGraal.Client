#pragma once

#include "Animation.h"
#include <map>
#include <string>
#include <filesystem>
#include <boost/algorithm/string.hpp>

class AnimationManager
{
public:
	static Animation *Get(const std::string &name);
	static void LoadFrom(const std::filesystem::path &path);

private:
	static void Load(const std::filesystem::path &path);

private:
	static std::map<std::string, Animation *> Animations;
};