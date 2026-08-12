#include "gui_progress_ctrl.hpp"

#include <algorithm>

void gui_progress_ctrl::set_progress(const float progress) {
    progress_ = std::clamp(progress, 0.0f, 1.0f);
}

void gui_progress_ctrl::draw() const {
    const auto profile = get_profile();
    if (!profile) {
        return;
    }

    const auto bounds = get_bounds();

    DrawRectangleRec(bounds, profile->fill_color);
    DrawRectangleRec({bounds.x, bounds.y, bounds.width * progress_, bounds.height}, profile->fill_color_hl);
    DrawRectangleLinesEx(bounds, static_cast<float>(profile->border_thickness), profile->border_color);

    draw_children();
}
