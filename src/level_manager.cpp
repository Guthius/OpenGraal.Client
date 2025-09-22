#include "level_manager.hpp"

#include <boost/algorithm/string.hpp>

#include "file_manager.hpp"

auto level_manager::Get(const std::string &name) -> level *
{
	const auto key = boost::to_lower_copy(name);
	const auto it = Levels.find(key);

	if (it == Levels.end())
	{
		return Load(name, key);
	}

	return it->second;
}

auto level_manager::Load(const std::string &name, const std::string &key) -> level *
{
	const auto path = file_manager::GetPath(name);

	if (path.empty())
	{
		TraceLog(LOG_ERROR, "Could not find level '%s'", name.c_str());

		Levels[key] = nullptr;

		return nullptr;
	}

	const auto level = level::Load(path);

	Levels[key] = level;

	if (level == nullptr)
	{
		TraceLog(LOG_ERROR, "Failed to load level '%s'", name.c_str());
	}

	return level;
}

std::map<std::string, level *> level_manager::Levels{};
