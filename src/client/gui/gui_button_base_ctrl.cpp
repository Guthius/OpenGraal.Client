#include "gui_button_base_ctrl.hpp"

void gui_button_base_ctrl::perform_click() {
    on_clicked();
}

void gui_button_base_ctrl::on_mouse_pressed(int button, Vector2 pt) {
    pressed_ = true;

    capture_mouse();
}

void gui_button_base_ctrl::on_mouse_released(int button, Vector2 pt) {
    const auto was_pressed = pressed_;

    pressed_ = false;

    if (was_pressed && contains(pt)) {
        perform_click();
    }

    release_mouse();
}

void gui_button_base_ctrl::on_clicked() {
    fire_clicked();
}

void gui_button_base_ctrl::fire_clicked() {
    if (clicked) {
        clicked();
    }
}
