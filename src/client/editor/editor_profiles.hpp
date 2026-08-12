#pragma once

#include "../gui/gui_button_ctrl.hpp"
#include "../gui/gui_control_profile.hpp"
#include "../gui/gui_text_ctrl.hpp"
#include "../gui/gui_text_edit_ctrl.hpp"

#include <memory>
#include <string>

namespace editor_profiles {
    auto body() -> const std::shared_ptr<gui_control_profile> &;
    auto toolbar() -> const std::shared_ptr<gui_control_profile> &;
    auto divider() -> const std::shared_ptr<gui_control_profile> &;
    auto canvas() -> const std::shared_ptr<gui_control_profile> &;
    auto panel() -> const std::shared_ptr<gui_control_profile> &;
    auto palette() -> const std::shared_ptr<gui_control_profile> &;
    auto button() -> const std::shared_ptr<gui_control_profile> &;
    auto tab() -> const std::shared_ptr<gui_control_profile> &;
    auto window() -> const std::shared_ptr<gui_control_profile> &;
    auto label() -> const std::shared_ptr<gui_control_profile> &;
    auto dialog_label() -> const std::shared_ptr<gui_control_profile> &;
    auto list() -> const std::shared_ptr<gui_control_profile> &;
    auto list_text() -> const std::shared_ptr<gui_control_profile> &;
    auto status() -> const std::shared_ptr<gui_control_profile> &;
    auto scrollbar() -> const std::shared_ptr<gui_control_profile> &;
    auto check() -> const std::shared_ptr<gui_control_profile> &;
    auto radio() -> const std::shared_ptr<gui_control_profile> &;
    auto edit() -> const std::shared_ptr<gui_control_profile> &;
    auto error_line() -> const std::shared_ptr<gui_control_profile> &;
    auto code() -> const std::shared_ptr<gui_control_profile> &;
    auto overlay() -> const std::shared_ptr<gui_control_profile> &;

    auto make_label(const std::string &text, Vector2 position, float width = 140) -> std::shared_ptr<gui_text_ctrl>;
    auto make_edit(Vector2 position, Vector2 size) -> std::shared_ptr<gui_text_edit_ctrl>;
    auto make_button(const std::string &text, float width) -> std::shared_ptr<gui_button_ctrl>;

    void centre_window(gui_control &window, Vector2 size);

    void begin_invert();
    void end_invert();
}
