#include "editor_script_dialog.hpp"

#include "editor_profiles.hpp"

#include <gs2/environment.hpp>
#include <gs2/script.hpp>

#include <format>
#include <sstream>

namespace {
    constexpr Vector2 dialog_size{520, 420};
}

auto editor_script_dialog::attach(const std::shared_ptr<gui_control> &root) -> void {
    window_ = std::make_shared<gui_window_ctrl>();
    window_->set_profile(editor_profiles::window());
    window_->set_size(dialog_size);
    window_->set_text("Edit npc action script");
    window_->set_visible(false);
    window_->set_can_resize(true);
    window_->set_min_size(dialog_size);
    window_->set_can_close(true);
    window_->set_can_maximize(true);
    window_->close_requested = [this] { close(); };

    window_->add_child(editor_profiles::make_label("Image:", {16, 40}, 60));

    image_edit_ = editor_profiles::make_edit({76, 38}, {300, 24});
    image_edit_->set_horiz_sizing(gui_horiz_sizing::width);

    auto browse = editor_profiles::make_button("Browse", 80);
    browse->set_position({388, 38});
    browse->set_horiz_sizing(gui_horiz_sizing::left);
    browse->clicked = [this] {
        if (browse_image) {
            browse_image();
        }
    };

    script_edit_ = std::make_shared<gui_ml_text_edit_ctrl>();
    script_edit_->set_profile(editor_profiles::code());
    script_edit_->set_scrollbar_profile(editor_profiles::scrollbar());
    script_edit_->set_position({16, 72});
    script_edit_->set_size({dialog_size.x - 32, dialog_size.y - 150});
    script_edit_->set_tab_spaces(2);
    script_edit_->set_line_height(18);
    script_edit_->set_horiz_sizing(gui_horiz_sizing::width);
    script_edit_->set_vert_sizing(gui_vert_sizing::height);

    error_edit_ = std::make_shared<gui_text_edit_ctrl>();
    error_edit_->set_profile(editor_profiles::error_line());
    error_edit_->set_read_only(true);
    error_edit_->set_position({16, dialog_size.y - 70});
    error_edit_->set_size({dialog_size.x - 32, 22});
    error_edit_->set_horiz_sizing(gui_horiz_sizing::width);
    error_edit_->set_vert_sizing(gui_vert_sizing::top);

    auto test = editor_profiles::make_button("Test", 90);
    test->set_position({16, dialog_size.y - 38});
    test->set_icon_size(20, 20);
    test->get_icon()->draw_image("icon_test.png", {0, 0});
    test->set_vert_sizing(gui_vert_sizing::top);
    test->clicked = [this] { run_test(); };

    auto close_button = editor_profiles::make_button("Close", 90);
    close_button->set_position({dialog_size.x - 106, dialog_size.y - 38});
    close_button->set_horiz_sizing(gui_horiz_sizing::left);
    close_button->set_vert_sizing(gui_vert_sizing::top);
    close_button->clicked = [this] { close(); };

    window_->add_child(image_edit_);
    window_->add_child(browse);
    window_->add_child(script_edit_);
    window_->add_child(error_edit_);
    window_->add_child(test);
    window_->add_child(close_button);

    root->add_child(window_);
}

void editor_script_dialog::show(const std::string &image, const std::string &script,
    std::function<void(const std::string &, const std::string &)> committed) {
    committed_ = std::move(committed);

    image_edit_->set_text(image);
    script_edit_->set_text(script);
    error_edit_->set_text("Error: (none)");

    if (!window_->is_maximized()) {
        editor_profiles::centre_window(*window_, window_->get_size());
    }

    if (open_modal) {
        open_modal(*window_);
    }

    script_edit_->focus();
}

auto editor_script_dialog::is_visible() const -> bool {
    return window_ && window_->is_visible();
}

void editor_script_dialog::set_image(const std::string &image) {
    if (image_edit_) {
        image_edit_->set_text(image);
    }
}

void editor_script_dialog::run_test() {
    og::gs2::environment env;
    std::istringstream is(script_edit_->get_text());

    if (const auto parsed = og::gs2::load_script(env, is)) {
        error_edit_->set_text("Error: (none)");
    } else {
        error_edit_->set_text(std::format("Error: line {}: {}", parsed.error().position.line, parsed.error().message));
    }
}

void editor_script_dialog::close() {
    if (!is_visible()) {
        return;
    }

    image_edit_->blur();
    script_edit_->blur();
    error_edit_->blur();

    const auto committed = std::move(committed_);

    if (close_modal) {
        close_modal(*window_);
    }

    if (committed) {
        committed(image_edit_->get_text(), script_edit_->get_text());
    }
}
