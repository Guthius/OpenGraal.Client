#pragma once

#include "../gui/gui_control.hpp"
#include "editor_state.hpp"

#include <functional>

class editor_canvas : public gui_control {
  public:
    void set_tab(editor_tab *tab) { tab_ = tab; }
    void set_show_grid(const bool on) { show_grid_ = on; }
    void set_hide_npcs(const bool on) { hide_npcs_ = on; }
    void set_mode(const editor_mode mode) { mode_ = mode; }
    void set_brush(const tile_clipboard *brush) { brush_ = brush; }

    void set_floating(const tile_clipboard *tiles, const int x, const int y) {
        floating_ = tiles;
        floating_x_ = x;
        floating_y_ = y;
    }

    [[nodiscard]] auto tab() const -> editor_tab * { return tab_; }
    [[nodiscard]] auto hover_tile() const -> Vector2 { return hover_tile_; }
    [[nodiscard]] auto npc_at(Vector2 level_tile) const -> int;

    void update(float dt) override;
    void draw() const override;

    std::function<void(int tile_x, int tile_y, og::shared::tile_id tile)> hovered;
    std::function<void()> view_changed;
    std::function<void(int button, Vector2 level_tile)> canvas_pressed;
    std::function<void(Vector2 level_tile)> canvas_moved;
    std::function<void(int button, Vector2 level_tile)> canvas_released;

  protected:
    void on_mouse_pressed(int button, Vector2 pt) override;
    void on_mouse_move(Vector2 pt) override;
    void on_mouse_released(int button, Vector2 pt) override;

  private:
    [[nodiscard]] auto level_tile_at(Vector2 pt) const -> Vector2;

    editor_tab *tab_ = nullptr;
    bool show_grid_ = false;
    bool hide_npcs_ = false;
    bool panning_ = false;
    bool move_cursor_ = false;
    Vector2 pan_start_{};
    Vector2 pan_origin_{};
    Vector2 hover_tile_{};
    editor_mode mode_ = editor_mode::none;
    const tile_clipboard *brush_ = nullptr;
    const tile_clipboard *floating_ = nullptr;
    int floating_x_ = 0;
    int floating_y_ = 0;
};
