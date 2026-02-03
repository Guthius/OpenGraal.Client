#include "gui_radio_ctrl.hpp"

void gui_radio_ctrl::on_clicked() {
    if (is_checked()) {
        return;
    }

    set_checked(true);

    const auto group = get_group();
    if (const auto parent = get_parent(); parent) {
        for (auto &child : parent->get_children()) {
            const auto radio_ctrl = dynamic_cast<gui_radio_ctrl *>(child.get());
            if (radio_ctrl && radio_ctrl != this &&
                radio_ctrl->get_group() == group &&
                radio_ctrl->is_checked()) {
                radio_ctrl->set_checked(false);
            }
        }
    }
}
