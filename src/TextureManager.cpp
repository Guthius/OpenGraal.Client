#include "TextureManager.h"

#include <boost/algorithm/string.hpp>

#include "FileManager.h"

TextureManager::TextureMap TextureManager::_textures;

auto TextureManager::Get(const std::string &fileName) -> Texture2D
{
	const auto key = boost::to_lower_copy(fileName);
	const auto it = _textures.find(key);

	if (it == _textures.end())
	{
		return Load(key);
	}

	return it->second;
}

auto TextureManager::Load(const std::string &key) -> Texture2D
{
	const auto path = FileManager::GetPath(key);

	if (path.empty())
	{
		_textures[key] = {};

		return {};
	}

	const auto texture = LoadTexture(path.string().c_str());

	_textures[key] = texture;

	return texture;
}
