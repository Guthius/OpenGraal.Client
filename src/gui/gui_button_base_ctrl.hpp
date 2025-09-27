#pragma once

#include "gui_text_ctrl.hpp"

class gui_button_base_ctrl : public gui_text_ctrl
{
public:
	void perform_click();

	[[nodiscard]] auto get_group() const -> int { return group_; }
	[[nodiscard]] auto is_checked() const -> bool { return checked_; }
	[[nodiscard]] auto is_pressed() const -> bool { return pressed_; }

	void set_group(const int group) { group_ = group; }
	void set_checked(const bool checked) { checked_ = checked; }

protected:
	void on_mouse_pressed(int button, Vector2 pt) override;
	void on_mouse_released(int button, Vector2 pt) override;

	virtual void on_clicked();

private:
	int group_ = 0;
	bool checked_ = false;
	bool pressed_ = false;
};
