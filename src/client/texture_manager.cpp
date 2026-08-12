#include "texture_manager.hpp"

#include "file_manager.hpp"
#include "network.hpp"

#include <map>
#include <shared/text.hpp>

namespace {
    std::map<std::string, Texture2D> loaded_textures;

    auto load_texture_from_file(const std::string &key) -> Texture2D {
        const auto path = find_file(key);

        if (path.empty()) {
            get_network().request_file(key);

            return loaded_textures[key] = {};
        }

        const auto texture = LoadTexture(path.string().c_str());

        return loaded_textures[key] = texture;
    }
}

auto load_texture(const std::string &filename) -> Texture2D {
    const auto key = og::shared::to_lower(filename);
    const auto iter = loaded_textures.find(key);

    if (iter == loaded_textures.end()) {
        return load_texture_from_file(key);
    }

    return iter->second;
}

void forget_texture(const std::string &filename) {
    loaded_textures.erase(og::shared::to_lower(filename));
}
