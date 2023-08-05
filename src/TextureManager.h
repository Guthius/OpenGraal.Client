#pragma once

#include <string>
#include <vector>
#include <map>
#include <raylib.h>

class TextureManager {
private:
	struct TextureInfo {
		std::string Name;
		Texture2D Texture;
		int RefCount;
	};

	typedef std::vector<TextureInfo> TextureInfoList;

public:
	static Texture2D Get(const std::string &name);
	static void Release(Texture2D texture);

private:
	static Texture2D Load(const std::string &name);

private:
	static TextureInfoList _textures;
};