#include "level_manager.hpp"

#include <map>
#include <boost/algorithm/string.hpp>

#include "file_manager.hpp"

namespace
{
	std::map<std::string, level *> loaded_levels;

	auto load_level_from_file(const std::string &key) -> level *
	{
		const auto path = find_file(key);

		if (path.empty())
		{
			TraceLog(LOG_ERROR, "Could not find level '%s'", key.c_str());

			loaded_levels[key] = nullptr;

			return nullptr;
		}

		const auto level = level::load(path);

		loaded_levels[key] = level;

		if (level == nullptr)
		{
			TraceLog(LOG_ERROR, "Failed to load level '%s'", key.c_str());
		}

		return level;
	}
}

auto load_level(const std::string &name) -> level *
{
	const auto key = boost::to_lower_copy(name);
	const auto iter = loaded_levels.find(key);

	if (iter == loaded_levels.end())
	{
		return load_level_from_file(key);
	}

	return iter->second;
}
