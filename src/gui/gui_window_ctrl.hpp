#pragma once

#include "gui_text_ctrl.hpp"

class gui_window_ctrl final : public gui_text_ctrl {
  public:
    void draw() const override;

    [[nodiscard]]
    auto is_draggable() const -> bool { return draggable_; }

    void set_draggable(const bool draggable) { draggable_ = draggable; }

  protected:
    void on_mouse_pressed(int button, Vector2 pt) override;
    void on_mouse_released(int button, Vector2 pt) override;
    void on_mouse_move(Vector2 pt) override;

  private:
    bool draggable_ = false;
    bool dragging_ = false;
    Vector2 drag_offset_{};
};
