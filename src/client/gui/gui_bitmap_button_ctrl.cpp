#include "gui_bitmap_button_ctrl.hpp"

#include "../texture_manager.hpp"

void gui_bitmap_button_ctrl::draw() const {
    const auto bounds = get_bounds();

    const auto &bitmap = [this]() -> const std::string & {
        if (is_pressed() && !pressed_.empty()) {
            return pressed_;
        }

        if (is_mouse_over() && !mouse_over_.empty()) {
            return mouse_over_;
        }

        return normal_;
    }();

    if (const auto texture = load_texture(bitmap); IsTextureValid(texture)) {
        DrawTexturePro(
            texture,
            {0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)},
            bounds, {0, 0}, 0.0f, WHITE);
    } else if (const auto profile = get_profile(); profile) {
        profile->draw_frame(bounds, is_mouse_over(), is_disabled());
        profile->draw_text(get_text(), bounds, alignment::center);
    }

    draw_children();
}
