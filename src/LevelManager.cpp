#include "LevelManager.h"

#include <boost/algorithm/string.hpp>

#include "FileManager.h"

auto LevelManager::Get(const std::string &name) -> Level *
{
	const auto key = boost::to_lower_copy(name);
	const auto it = Levels.find(key);

	if (it == Levels.end())
	{
		return Load(name, key);
	}

	return it->second;
}

auto LevelManager::Load(const std::string &name, const std::string &key) -> Level *
{
	const auto path = FileManager::GetPath(name);

	if (path.empty())
	{
		TraceLog(LOG_ERROR, "Could not find level '%s'", name.c_str());

		Levels[key] = nullptr;

		return nullptr;
	}

	const auto level = Level::Load(path);

	Levels[key] = level;

	if (level == nullptr)
	{
		TraceLog(LOG_ERROR, "Failed to load level '%s'", name.c_str());
	}

	return level;
}

std::map<std::string, Level *> LevelManager::Levels{};
