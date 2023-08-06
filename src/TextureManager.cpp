#include "TextureManager.h"
#include "FileManager.h"

#include <boost/algorithm/string.hpp>

TextureManager::TextureMap TextureManager::_textures;

Texture2D TextureManager::Get(const std::string &fileName)
{
	auto key = boost::to_lower_copy(fileName);

	auto it = _textures.find(key);

	if (it == _textures.end())
	{
		return Load(key);
	}

	return it->second;
}

Texture2D TextureManager::Load(const std::string &key)
{
	auto path = FileManager::GetPath(key);

	if (path.empty())
	{
		_textures[key] = {};

		return {};
	}

	auto texture = LoadTexture(path.string().c_str());

	_textures[key] = texture;

	return texture;
}