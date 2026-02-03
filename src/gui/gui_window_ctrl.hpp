#pragma once

#include "gui_text_ctrl.hpp"

class gui_window_ctrl final : public gui_text_ctrl {
  public:
    void draw() const override;
};
