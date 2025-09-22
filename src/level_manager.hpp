#pragma once

#include <map>
#include <string>

#include "level.hpp"

class level_manager
{
public:
	static auto Get(const std::string &name) -> level *;

private:
	static auto Load(const std::string &name, const std::string &key) -> level *;

	static std::map<std::string, level *> Levels;
};
