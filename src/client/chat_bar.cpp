#include "chat_bar.hpp"

#include "dev_input.hpp"
#include "gui/gui.hpp"
#include "gui/gui_control_profile.hpp"

#include <raylib.h>

namespace {
    constexpr float bar_height = 22.0f;
    constexpr float bar_margin = 4.0f;

    chat_bar instance;
}

auto get_chat_bar() -> chat_bar & {
    return instance;
}

void chat_bar::attach(const std::shared_ptr<gui_control> &root) {
    const auto profile = std::make_shared<gui_control_profile>();
    profile->set_texture("guiblue_textedit.png");
    profile->set_font("LiberationSans-Regular.ttf");
    profile->fill_color = {20, 30, 60, 210};
    profile->fill_color_hl = {20, 30, 60, 210};

    input_ = std::make_shared<gui_text_edit_ctrl>();
    input_->set_profile(profile);
    input_->set_visible(false);

    root->add_child(input_);

    layout(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
}

void chat_bar::layout(const float screen_width, const float screen_height) const {
    if (!input_) {
        return;
    }

    input_->set_position({bar_margin, screen_height - bar_height - bar_margin});
    input_->set_size({screen_width - (bar_margin * 2), bar_height});
}

void chat_bar::open() {
    if (!input_ || open_) {
        return;
    }

    open_ = true;

    input_->set_visible(true);
    input_->set_text("");
    input_->bring_to_front();
    input_->focus();
}

void chat_bar::close() {
    if (!input_ || !open_) {
        return;
    }

    open_ = false;

    input_->blur();
    input_->set_text("");
    input_->set_visible(false);
}

void chat_bar::toggle() {
    open_ ? close() : open();
}

auto chat_bar::update() -> bool {
    if (!input_) {
        return false;
    }

    layout(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));

    if (!open_ && gui_has_focus()) {
        return false;
    }

    if (dev_input::key_pressed(KEY_TAB)) {
        toggle();

        return true;
    }

    if (!open_) {
        return false;
    }

    if (dev_input::key_pressed(KEY_ESCAPE)) {
        close();

        return true;
    }

    if (dev_input::key_pressed(KEY_ENTER) || dev_input::key_pressed(KEY_KP_ENTER)) {
        if (const auto text = input_->get_text(); !text.empty() && submit_handler_) {
            submit_handler_(text);
        }

        close();

        return true;
    }

    return true;
}
