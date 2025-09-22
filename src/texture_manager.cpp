#include "texture_manager.hpp"

#include <boost/algorithm/string.hpp>

#include "file_manager.hpp"

texture_manager::TextureMap texture_manager::_textures;

auto texture_manager::Get(const std::string &fileName) -> Texture2D
{
	const auto key = boost::to_lower_copy(fileName);
	const auto it = _textures.find(key);

	if (it == _textures.end())
	{
		return Load(key);
	}

	return it->second;
}

auto texture_manager::Load(const std::string &key) -> Texture2D
{
	const auto path = file_manager::GetPath(key);

	if (path.empty())
	{
		_textures[key] = {};

		return {};
	}

	const auto texture = LoadTexture(path.string().c_str());

	_textures[key] = texture;

	return texture;
}
