#include "tileset_manager.hpp"

#include <map>
#include <boost/algorithm/string.hpp>

namespace {
    std::map<std::string, tileset *> loaded_tilesets;
}

auto load_tileset(const std::string &texture_filename) -> tileset * {
    const auto key = boost::to_lower_copy(texture_filename);

    const auto iter = loaded_tilesets.find(key);
    if (iter != loaded_tilesets.end()) {
        return iter->second;
    }

    const auto tileset = new ::tileset(texture_filename);

    loaded_tilesets[key] = tileset;

    return tileset;
}
