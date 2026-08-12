#pragma once

#include "editor_state.hpp"

namespace editor_ops {
    [[nodiscard]] auto board(og::shared::level_data &level) -> std::vector<og::shared::tile_id> &;

    [[nodiscard]] auto copy_rect(og::shared::level_data &level, int x, int y, int width, int height) -> tile_clipboard;

    void paste(og::shared::level_data &level, int x, int y, const tile_clipboard &source);
    void fill_rect(og::shared::level_data &level, int x, int y, int width, int height, og::shared::tile_id tile);
    void flood_fill(og::shared::level_data &level, int x, int y, og::shared::tile_id tile);

    [[nodiscard]] auto snapshot(og::shared::level_data &level) -> tile_patch;
}
