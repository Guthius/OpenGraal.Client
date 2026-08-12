#pragma once

#include <shared/level.hpp>

#include <deque>
#include <raylib.h>
#include <string>
#include <variant>
#include <vector>

inline constexpr og::shared::tile_id editor_default_tile = 2047;

enum class editor_mode : uint8_t {
    none,
    tile_placement,
    npc_placement
};

struct tile_patch {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::vector<og::shared::tile_id> before;
};

struct npcs_snapshot {
    std::vector<og::shared::level_npc> before;
};

struct links_snapshot {
    std::vector<og::shared::level_link> before;
};

struct signs_snapshot {
    std::vector<og::shared::level_sign> before;
};

using undo_entry = std::variant<tile_patch, npcs_snapshot, links_snapshot, signs_snapshot>;

struct tile_selection {
    bool active = false;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct tile_clipboard {
    int width = 0;
    int height = 0;
    std::vector<og::shared::tile_id> tiles;
};

struct canvas_view {
    float pan_x = 0.0f;
    float pan_y = 0.0f;
    float zoom = 1.0f;

    [[nodiscard]] auto level_to_screen(Vector2 level_px) const -> Vector2 {
        return {(level_px.x - pan_x) * zoom, (level_px.y - pan_y) * zoom};
    }

    [[nodiscard]] auto screen_to_level(Vector2 screen_px) const -> Vector2 {
        return {(screen_px.x / zoom) + pan_x, (screen_px.y / zoom) + pan_y};
    }

    auto zoom_step(int direction, Vector2 anchor_screen) -> void;
};

struct editor_tab {
    std::string title;
    std::string server_name;
    og::shared::level_data level;
    std::deque<undo_entry> undo;
    bool dirty = false;
    bool converted = false;
    canvas_view view;
    tile_selection selection;
    int selected_npc = -1;
};
