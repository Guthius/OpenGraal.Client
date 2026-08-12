#pragma once

#include <string>

#include "gui_button_base_ctrl.hpp"

class gui_bitmap_button_ctrl final : public gui_button_base_ctrl {
  public:
    [[nodiscard]] auto get_normal_bitmap() const -> const std::string & { return normal_; }
    [[nodiscard]] auto get_mouse_over_bitmap() const -> const std::string & { return mouse_over_; }
    [[nodiscard]] auto get_pressed_bitmap() const -> const std::string & { return pressed_; }

    void set_normal_bitmap(std::string bitmap) { normal_ = std::move(bitmap); }
    void set_mouse_over_bitmap(std::string bitmap) { mouse_over_ = std::move(bitmap); }
    void set_pressed_bitmap(std::string bitmap) { pressed_ = std::move(bitmap); }

    void draw() const override;

  private:
    std::string normal_;
    std::string mouse_over_;
    std::string pressed_;
};
