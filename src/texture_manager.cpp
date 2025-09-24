#include "texture_manager.hpp"

#include <map>
#include <boost/algorithm/string.hpp>

#include "file_manager.hpp"

namespace
{
	std::map<std::string, Texture2D> loaded_textures;

	auto load_texture_from_file(const std::string &key) -> Texture2D
	{
		const auto path = find_file(key);

		if (path.empty())
		{
			loaded_textures[key] = {};

			return {};
		}

		const auto texture = LoadTexture(path.string().c_str());

		loaded_textures[key] = texture;

		return texture;
	}
}

auto load_texture(const std::string &filename) -> Texture2D
{
	const auto key = boost::to_lower_copy(filename);
	const auto iter = loaded_textures.find(key);

	if (iter == loaded_textures.end())
	{
		return load_texture_from_file(key);
	}

	return iter->second;
}
