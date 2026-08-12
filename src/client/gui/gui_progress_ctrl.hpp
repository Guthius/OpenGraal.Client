#pragma once

#include "gui_control.hpp"

class gui_progress_ctrl : public gui_control {
  public:
    [[nodiscard]] auto get_progress() const -> float { return progress_; }

    void set_progress(float progress);

    void draw() const override;

  private:
    float progress_ = 0.0f;
};
