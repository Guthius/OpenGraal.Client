#include "TextureManager.h"

TextureManager::TextureInfoList TextureManager::_textures;

Texture2D TextureManager::Load(const std::string &name)
{
	auto fileName = TextToLower(name.c_str());
	auto texture = LoadTexture(fileName);

	_textures.push_back({name, texture, 1});

	return texture;
}

Texture2D TextureManager::Get(const std::string &name)
{
	for (auto &i : _textures)
	{
		if (i.Name == name)
		{
			i.RefCount++;

			return i.Texture;
		}
	}

	return Load(name);
}

void TextureManager::Release(Texture2D texture)
{
	for (auto it = _textures.begin(); it != _textures.end();)
	{
		if (it->Texture.id != texture.id)
		{
			++it;

			continue;
		}

		it->RefCount--;

		if (it->RefCount <= 0)
		{
			UnloadTexture(it->Texture);

			_textures.erase(it);
		}
	}
}