#pragma once

#include "gui_control.hpp"

class gui_scrollbar_ctrl : public gui_control {
  public:
    void set_vertical(const bool vertical) { vertical_ = vertical; }
    void set_range(float content, float view);
    void set_value(float value);

    [[nodiscard]] auto get_value() const -> float { return value_; }

    void draw() const override;

    std::function<void(float)> changed;

  protected:
    void on_mouse_pressed(int button, Vector2 pt) override;
    void on_mouse_move(Vector2 pt) override;
    void on_mouse_released(int button, Vector2 pt) override;
    void on_mouse_exit() override;

  private:
    [[nodiscard]] auto button_length() const -> float;
    [[nodiscard]] auto track_length() const -> float;
    [[nodiscard]] auto thumb_length() const -> float;
    [[nodiscard]] auto thumb_offset() const -> float;
    [[nodiscard]] auto max_value() const -> float;

    void scroll_to(float value);

    bool vertical_ = false;
    float content_ = 1.0f;
    float view_ = 1.0f;
    float value_ = 0.0f;
    bool dragging_ = false;
    float drag_grab_ = 0.0f;
    float hover_along_ = -1.0f;
};
