#pragma once

#include "Level.h"

#include <map>
#include <string>

class LevelManager
{
public:
	static Level *Get(const std::string &name);

private:
	static Level *Load(const std::string &name, const std::string &key);

private:
	static std::map<std::string, Level *> Levels;
};