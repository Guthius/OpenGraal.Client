#pragma once

#include <map>
#include <raylib.h>
#include <string>

class TextureManager
{
	using TextureMap = std::map<std::string, Texture2D>;

public:
	static auto Get(const std::string &fileName) -> Texture2D;

private:
	static auto Load(const std::string &key) -> Texture2D;

	static TextureMap _textures;
};
