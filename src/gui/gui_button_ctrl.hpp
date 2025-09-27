#pragma once

#include <functional>

#include "gui_button_base_ctrl.hpp"

class gui_button_ctrl final : public gui_button_base_ctrl
{
public:
	void draw() const override;

	std::function<void()> clicked;

protected:
	void on_clicked() override;
};
