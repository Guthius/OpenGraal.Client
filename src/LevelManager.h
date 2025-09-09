#pragma once

#include <map>
#include <string>

#include "Level.h"

class LevelManager
{
public:
	static auto Get(const std::string &name) -> Level *;

private:
	static auto Load(const std::string &name, const std::string &key) -> Level *;

	static std::map<std::string, Level *> Levels;
};
