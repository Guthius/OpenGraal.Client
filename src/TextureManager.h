#pragma once

#include <string>
#include <vector>
#include <map>
#include <raylib.h>

class TextureManager {
private:
	typedef std::map<std::string, Texture2D> TextureMap;

public:
	static Texture2D Get(const std::string &fileName);

private:
	static Texture2D Load(const std::string &key);

private:
	static TextureMap _textures;
};