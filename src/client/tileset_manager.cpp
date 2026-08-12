#include "tileset_manager.hpp"

#include "file_manager.hpp"
#include "network.hpp"

#include <map>
#include <shared/text.hpp>
#include <vector>

namespace {
    struct tile_def {
        std::string image;
        std::string prefix;
        int format = og::shared::tileset_format_pics1;
    };

    std::map<std::string, tileset *> loaded_tilesets;
    std::vector<tile_def> tile_defs;
}

auto load_tileset(const std::string &texture_filename, const int format) -> tileset * {
    const auto key = og::shared::to_lower(texture_filename);

    const auto iter = loaded_tilesets.find(key);
    if (iter != loaded_tilesets.end()) {
        return iter->second;
    }

    auto *const tileset = new ::tileset(texture_filename, format);

    loaded_tilesets[key] = tileset;

    return tileset;
}

void add_tile_def(const std::string &texture_filename, const std::string &level_prefix, const int format) {
    const auto prefix = og::shared::to_lower(level_prefix);

    std::erase_if(tile_defs, [&prefix](const tile_def &def) { return def.prefix == prefix; });

    tile_defs.push_back({og::shared::to_lower(texture_filename), prefix, format});
}

void clear_tile_defs() {
    tile_defs.clear();
}

auto tileset_for_level(const std::string &level_name) -> tileset * {
    const auto name = og::shared::to_lower(level_name);
    const tile_def *best = nullptr;

    for (const auto &def : tile_defs) {
        if (!name.starts_with(def.prefix)) {
            continue;
        }

        if (best == nullptr || def.prefix.size() > best->prefix.size()) {
            best = &def;
        }
    }

    if (best == nullptr) {
        return load_tileset("pics1.png");
    }

    if (!has_file(best->image)) {
        get_network().request_file(best->image);

        return load_tileset("pics1.png");
    }

    return load_tileset(best->image, best->format);
}
