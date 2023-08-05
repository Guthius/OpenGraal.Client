#include "SoundManager.h"

#include <boost/algorithm/string.hpp>

std::map<std::string, Sound> SoundManager::Sounds{};

Sound SoundManager::Get(const std::string &fileName)
{
	auto key = boost::to_lower_copy(fileName);

	auto it = Sounds.find(key);

	if (it != Sounds.end())
	{
		return it->second;
	}

	return TryLoad(fileName);
}

Sound SoundManager::TryLoad(const std::string &fileName)
{
	auto sound = LoadSound(fileName.c_str());

	auto key = boost::to_lower_copy(fileName);

	Sounds[key] = sound;

	return sound;
}