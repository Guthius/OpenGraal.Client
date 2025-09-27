#include "gui_text_ctrl.hpp"

void gui_text_ctrl::draw() const
{
	if (const auto profile = get_profile(); profile)
	{
		profile->draw_text(text_, get_bounds(), alignment::left);
	}
}
