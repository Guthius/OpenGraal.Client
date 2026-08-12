#pragma once

#include "gui_control.hpp"

class gui_slider_ctrl : public gui_control {
  public:
    [[nodiscard]] auto get_value() const -> float { return value_; }
    [[nodiscard]] auto get_minimum() const -> float { return minimum_; }
    [[nodiscard]] auto get_maximum() const -> float { return maximum_; }

    void set_value(float value);
    void set_range(float minimum, float maximum);

    void draw() const override;

  protected:
    void on_mouse_pressed(int button, Vector2 pt) override;
    void on_mouse_move(Vector2 pt) override;
    void on_mouse_released(int button, Vector2 pt) override;

  private:
    void track(Vector2 pt);

    float value_ = 0.0f;
    float minimum_ = 0.0f;
    float maximum_ = 1.0f;
    bool dragging_ = false;
};
