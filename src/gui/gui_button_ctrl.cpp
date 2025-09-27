#include "gui_button_ctrl.hpp"

namespace
{
	nine_patch state_normal{0, 1, 2, 3, 4, 5, 6, 7, 8};
	nine_patch state_mouse_over{9, 10, 11, 12, 13, 14, 15, 16, 17};
	nine_patch state_pressed{18, 19, 20, 21, 22, 23, 24, 25, 26};
	nine_patch state_disabled{27, 28, 29, 30, 31, 32, 33, 34, 35};
}

void gui_button_ctrl::draw() const
{
	auto get_state_nine_patch = [&]() -> const nine_patch & {
		if (is_disabled()) return state_disabled;
		if (is_pressed()) return state_pressed;
		if (is_mouse_over()) return state_mouse_over;
		return state_normal;
	};

	if (const auto profile = get_profile(); profile)
	{
		profile->draw_nine_patch(get_bounds(), get_state_nine_patch());
		profile->draw_text(get_text(), get_bounds(), alignment::center);
	}
}

void gui_button_ctrl::on_clicked()
{
	if (clicked)
	{
		clicked();
	}
}
