#include "SoundManager.h"

#include <boost/algorithm/string.hpp>

#include "FileManager.h"

SoundManager::SoundMap SoundManager::Sounds{};

auto SoundManager::Get(const std::string &fileName) -> Sound
{
	const auto key = boost::to_lower_copy(fileName);

	if (const auto it = Sounds.find(key); it != Sounds.end())
	{
		return it->second;
	}

	return Load(key);
}

auto SoundManager::Load(const std::string &key) -> Sound
{
	const auto path = FileManager::GetPath(key);

	if (path.empty())
	{
		Sounds[key] = {};

		return {};
	}

	const auto sound = LoadSound(path.string().c_str());

	Sounds[key] = sound;

	return sound;
}
