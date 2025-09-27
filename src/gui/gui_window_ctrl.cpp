#include "gui_window_ctrl.hpp"

namespace
{
	nine_patch state_normal{12, 14, 13, 18, 23, 19, 20, 21, 22};
	nine_patch state_disabled{15, 17, 16, 18, 23, 19, 20, 21, 22};
}

void gui_window_ctrl::draw() const
{
	auto get_nine_patch = [&]() -> const nine_patch & {
		return is_disabled() ? state_disabled : state_normal;
	};

	if (const auto profile = get_profile(); profile)
	{
		const auto &bounds = get_bounds();
		const auto &nine_patch = get_nine_patch();
		const auto &title_bar_sprite = profile->get_sprite_rect(nine_patch.t);

		profile->draw_nine_patch(bounds, nine_patch);

		const auto text_rect = Rectangle{
			.x = bounds.x + 10,
			.y = bounds.y,
			.width = bounds.width - 20,
			.height = title_bar_sprite.height
		};

		profile->draw_text(get_text(), text_rect, alignment::left);
	}

	draw_children();
}
