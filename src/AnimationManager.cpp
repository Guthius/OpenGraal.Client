#include "AnimationManager.h"

#include <boost/algorithm/string.hpp>

std::map<std::string, Animation *> AnimationManager::Animations{};

auto AnimationManager::Get(const std::string &name) -> Animation *
{
	const auto key = boost::to_lower_copy(name);
	const auto it = Animations.find(key);

	if (it == Animations.end())
	{
		return nullptr;
	}

	return it->second;
}

void AnimationManager::LoadFrom(const std::filesystem::path &path)
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

		Load(file.path());
	}
}

void AnimationManager::Load(const std::filesystem::path &path)
{
	const auto animation = new Animation();

	animation->Load(path);

	const auto key = boost::to_lower_copy(path.stem().string());

	Animations[key] = animation;
}
