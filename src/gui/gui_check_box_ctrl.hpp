#pragma once
#include "gui_button_base_ctrl.hpp"

class gui_check_box_ctrl : public gui_button_base_ctrl
{
public:
	void draw() const override;

protected:
	void on_clicked() override;
};