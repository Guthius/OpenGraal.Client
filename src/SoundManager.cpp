#include "SoundManager.h"
#include "FileManager.h"

#include <boost/algorithm/string.hpp>

SoundManager::SoundMap SoundManager::Sounds{};

Sound SoundManager::Get(const std::string &fileName)
{
	auto key = boost::to_lower_copy(fileName);

	auto it = Sounds.find(key);

	if (it != Sounds.end())
	{
		return it->second;
	}

	return Load(key);
}

Sound SoundManager::Load(const std::string &key)
{
	auto path = FileManager::GetPath(key);

	if (path.empty())
	{
		Sounds[key] = {};

		return {};
	}

	auto sound = LoadSound(path.string().c_str());

	Sounds[key] = sound;

	return sound;
}