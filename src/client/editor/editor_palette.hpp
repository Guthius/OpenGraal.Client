#pragma once

#include "../gui/gui_control.hpp"
#include "../tileset.hpp"
#include "editor_state.hpp"

#include <functional>
#include <utility>

class editor_palette : public gui_control {
  public:
    void set_background_tile(const og::shared::tile_id tile) { background_tile_ = tile; }
    [[nodiscard]] auto background_tile() const -> og::shared::tile_id { return background_tile_; }

    std::function<const tileset *()> tileset_source;

    void update(float dt) override;
    void draw() const override;

    std::function<void(const tile_clipboard &brush)> brush_selected;
    std::function<void(og::shared::tile_id tile)> background_picked;

  protected:
    void on_mouse_pressed(int button, Vector2 pt) override;
    void on_mouse_move(Vector2 pt) override;
    void on_mouse_released(int button, Vector2 pt) override;

  private:
    [[nodiscard]] auto cell_at(Vector2 pt) const -> std::pair<int, int>;

    og::shared::tile_id background_tile_ = editor_default_tile;
    canvas_view view_;

    bool selecting_ = false;
    bool panning_ = false;
    Vector2 pan_start_{};
    Vector2 pan_origin_{};
    int anchor_col_ = 0;
    int anchor_row_ = 0;
    int cursor_col_ = 0;
    int cursor_row_ = 0;
    double last_click_at_ = 0.0;
    int last_click_col_ = -1;
    int last_click_row_ = -1;
};
