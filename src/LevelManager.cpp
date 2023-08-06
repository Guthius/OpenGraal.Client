#include "LevelManager.h"
#include "FileManager.h"

#include <boost/algorithm/string.hpp>

std::map<std::string, Level *> LevelManager::Levels{};

Level *LevelManager::Get(const std::string &name)
{
	auto key = boost::to_lower_copy(name);
	auto it = Levels.find(key);

	if (it == Levels.end())
	{
		return Load(name, key);
	}

	return it->second;
}

Level *LevelManager::Load(const std::string &name, const std::string &key)
{
	auto path = FileManager::GetPath(name);

	if (path.empty())
	{
		TraceLog(LOG_ERROR, "Could not find level '%s'", name.c_str());

		Levels[key] = nullptr;

		return nullptr;
	}

	auto level = Level::Load(path);

	Levels[key] = level;

	if (level == nullptr)
	{
		TraceLog(LOG_ERROR, "Failed to load level '%s'", name.c_str());
	}

	return level;
}