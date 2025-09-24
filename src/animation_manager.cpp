#include "animation_manager.hpp"

#include <map>
#include <boost/algorithm/string.hpp>

namespace
{
	std::map<std::string, animation *> loaded_animations;

	void load_animation_from_file(const std::filesystem::path &path)
	{
		const auto animation = new ::animation();

		animation->load_file(path);

		const auto key = boost::to_lower_copy(path.stem().string());

		loaded_animations[key] = animation;
	}
}

auto load_animation(const std::string &name) -> animation *
{
	const auto key = boost::to_lower_copy(name);
	const auto iter = loaded_animations.find(key);

	if (iter == loaded_animations.end())
	{
		return nullptr;
	}

	return iter->second;
}

void load_animations_from_directory(const std::filesystem::path &path)
{
	if (!is_directory(path))
	{
		return;
	}

	for (const auto &file: std::filesystem::directory_iterator(path))
	{
		if (!file.is_regular_file())
		{
			continue;
		}

		if (auto ext = boost::to_lower_copy(file.path().extension().string()); ext != ".gani")
		{
			continue;
		}

		load_animation_from_file(file.path());
	}
}
