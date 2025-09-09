#pragma once

#include <map>
#include <raylib.h>
#include <string>

class SoundManager
{
	using SoundMap = std::map<std::string, Sound>;

public:
	static auto Get(const std::string &fileName) -> Sound;

private:
	static auto Load(const std::string &key) -> Sound;

	static SoundMap Sounds;
};
