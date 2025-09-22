#include "sound_manager.hpp"

#include <boost/algorithm/string.hpp>

#include "file_manager.hpp"

sound_manager::SoundMap sound_manager::Sounds{};

auto sound_manager::Get(const std::string &fileName) -> Sound
{
	const auto key = boost::to_lower_copy(fileName);

	if (const auto it = Sounds.find(key); it != Sounds.end())
	{
		return it->second;
	}

	return Load(key);
}

auto sound_manager::Load(const std::string &key) -> Sound
{
	const auto path = file_manager::GetPath(key);

	if (path.empty())
	{
		Sounds[key] = {};

		return {};
	}

	const auto sound = LoadSound(path.string().c_str());

	Sounds[key] = sound;

	return sound;
}
