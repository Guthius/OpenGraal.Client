#include "AnimationManager.h"

std::map<std::string, Animation *> AnimationManager::Animations{};

Animation *AnimationManager::Get(const std::string &name)
{
	auto key = boost::to_lower_copy(name);
	auto it = Animations.find(key);

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

		auto ext = boost::to_lower_copy(file.path().extension().string());
		if (ext != ".gani")
		{
			continue;
		}

		Load(file.path());
	}
}

void AnimationManager::Load(const std::filesystem::path &path)
{
	auto animation = new Animation();

	animation->Load(path);

	auto key = boost::to_lower_copy(path.stem().string());

	Animations[key] = animation;
}