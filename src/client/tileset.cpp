#include "tileset.hpp"

#include "texture_manager.hpp"

#include <shared/tile_defs.hpp>

tileset::tileset(const std::string &filename, const int format)
    : format_(format), texture_(load_texture(filename)) {
    tile_width_ = 16.0f / static_cast<float>(texture_.width);
    tile_height_ = 16.0f / static_cast<float>(texture_.height);

    const auto tiles_x = texture_.width / 16;
    const auto tiles_y = texture_.height / 16;

    tile_count_ = tiles_x * tiles_y;

    if (format != og::shared::tileset_format_pics1) {
        defs_ = og::shared::tile_defs_for_format(format, tile_count_);

        if (defs_.empty()) {
            TraceLog(LOG_WARNING, "TILESET: %s has no layout for format %d; nothing blocks", filename.c_str(), format);
        }

        return;
    }

    if (auto defs = og::shared::load_tile_defs("arrays.dat", tile_count_)) {
        defs_ = std::move(*defs);
    } else {
        TraceLog(LOG_WARNING, "TILESET: %s", defs.error().c_str());
    }
}

auto tileset::get_tile_type(const int tile_id) const -> int {
    return defs_.type_of(tile_id);
}
