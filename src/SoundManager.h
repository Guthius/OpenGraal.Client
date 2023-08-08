#pragma once

#include <string>
#include <map>
#include <raylib.h>

class SoundManager
{
private:
	typedef std::map<std::string, Sound> SoundMap;

public:
	static Sound Get(const std::string &fileName);

private:
	static Sound Load(const std::string &key);

private:
	static SoundMap Sounds;
};
