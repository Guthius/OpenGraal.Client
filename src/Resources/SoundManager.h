#pragma once

#include <string>
#include <map>
#include <raylib.h>

class SoundManager
{
public:
	static Sound Get(const std::string &fileName);

private:
	static Sound TryLoad(const std::string &fileName);

private:
	static std::map<std::string, Sound> Sounds;
};
