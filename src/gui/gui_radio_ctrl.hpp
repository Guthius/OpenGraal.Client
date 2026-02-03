#pragma once

#include "gui_check_box_ctrl.hpp"

class gui_radio_ctrl final : public gui_check_box_ctrl {
  protected:
    void on_clicked() override;
};
