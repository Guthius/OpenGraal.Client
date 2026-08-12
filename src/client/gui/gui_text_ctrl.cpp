#include "gui_text_ctrl.hpp"

void gui_text_ctrl::set_text(const std::string &text) {
    text_ = text;

    if (const auto profile = get_profile(); fit_to_text_ && profile) {
        set_size(profile->measure_text(text_));
    }
}

void gui_text_ctrl::draw() const {
    if (const auto profile = get_profile(); profile) {
        profile->draw_text(text_, get_bounds(), alignment::left);
    }
}
