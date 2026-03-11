#pragma once

#include "gui_text_ctrl.hpp"

class gui_window_ctrl final : public gui_text_ctrl {
  protected:
    void draw() const override;
};
