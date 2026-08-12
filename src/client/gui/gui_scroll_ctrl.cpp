#include "gui_scroll_ctrl.hpp"

void gui_scroll_ctrl::draw() const {
    const auto bounds = get_bounds();

    if (const auto profile = get_profile(); profile) {
        profile->draw_frame(bounds, is_mouse_over(), is_disabled());
    }

    const auto clip = local_to_global_coord({0, 0});

    BeginScissorMode(
        static_cast<int>(clip.x), static_cast<int>(clip.y),
        static_cast<int>(bounds.width), static_cast<int>(bounds.height));

    draw_children();

    EndScissorMode();
}
