#include "editor_file_browser.hpp"

#include "editor_profiles.hpp"

namespace {
    constexpr Vector2 browser_size{380, 420};
}

auto editor_file_browser::attach(const std::shared_ptr<gui_control> &root) -> void {
    window_ = std::make_shared<gui_window_ctrl>();
    window_->set_profile(editor_profiles::window());
    window_->set_size(browser_size);
    window_->set_visible(false);

    list_ = std::make_shared<gui_text_list_ctrl>();
    list_->set_profile(editor_profiles::list());
    list_->set_text_profile(editor_profiles::list_text());
    list_->set_position({16, 40});
    list_->set_size({browser_size.x - 32, browser_size.y - 120});
    list_->set_columns({4});
    list_->selected = [this](int, const std::string &text, int) {
        if (name_edit_) {
            name_edit_->set_text(text);
        }
    };

    name_edit_ = editor_profiles::make_edit({16, browser_size.y - 72}, {browser_size.x - 32, 24});

    auto ok = editor_profiles::make_button("OK", 100);
    ok->set_position({(browser_size.x / 2) - 108, browser_size.y - 38});
    ok->clicked = [this] { accept(); };

    auto cancel = editor_profiles::make_button("Cancel", 100);
    cancel->set_position({(browser_size.x / 2) + 8, browser_size.y - 38});
    cancel->clicked = [this] { close(); };

    name_edit_->submitted = [this] { accept(); };

    window_->add_child(list_);
    window_->add_child(name_edit_);
    window_->add_child(ok);
    window_->add_child(cancel);

    root->add_child(window_);
}

void editor_file_browser::show(const int category, const bool save_mode, const std::string &initial_name,
    std::function<void(const std::string &name)> chosen) {
    category_ = category;
    save_mode_ = save_mode;
    chosen_ = std::move(chosen);

    window_->set_text(save_mode ? "Save level as" : "Open");
    name_edit_->set_text(initial_name);
    list_->clear_rows();

    editor_profiles::centre_window(*window_, browser_size);

    if (open_modal) {
        open_modal(*window_);
    }

    if (request_listing) {
        request_listing(category);
    }

    name_edit_->focus();
}

void editor_file_browser::set_listing(const int category, const std::vector<std::string> &names) {
    if (!is_visible() || category != category_) {
        return;
    }

    list_->clear_rows();

    auto id = 0;
    for (const auto &name : names) {
        list_->add_row(id++, name);
    }
}

auto editor_file_browser::is_visible() const -> bool {
    return window_ && window_->is_visible();
}

void editor_file_browser::accept() {
    auto name = name_edit_->get_text();
    if (name.empty()) {
        return;
    }

    if (save_mode_ && !name.contains('.')) {
        name += ".nw";
    }

    const auto chosen = std::move(chosen_);
    close();

    if (chosen) {
        chosen(name);
    }
}

void editor_file_browser::close() {
    if (!is_visible()) {
        return;
    }

    name_edit_->blur();
    chosen_ = nullptr;

    if (close_modal) {
        close_modal(*window_);
    }
}
