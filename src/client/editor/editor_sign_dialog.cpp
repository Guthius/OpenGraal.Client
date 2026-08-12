#include "editor_sign_dialog.hpp"

#include "editor_profiles.hpp"

#include <shared/text.hpp>

#include <format>

namespace {
    constexpr Vector2 dialog_size{420, 420};
}

auto editor_sign_dialog::attach(const std::shared_ptr<gui_control> &root) -> void {
    window_ = std::make_shared<gui_window_ctrl>();
    window_->set_profile(editor_profiles::window());
    window_->set_size(dialog_size);
    window_->set_text("Edit signs");
    window_->set_visible(false);

    window_->add_child(editor_profiles::make_label("Select a sign to edit:", {16, 34}, 200));

    list_ = std::make_shared<gui_text_list_ctrl>();
    list_->set_profile(editor_profiles::list());
    list_->set_text_profile(editor_profiles::list_text());
    list_->set_position({16, 56});
    list_->set_size({dialog_size.x - 32, 130});
    list_->set_columns({4, 90});
    list_->selected = [this](int, const std::string &, int) {
        commit_selected();
        load_selected();
    };

    auto add = editor_profiles::make_button("New", 70);
    add->set_position({16, 196});
    add->clicked = [this] {
        if (tab_ == nullptr) {
            return;
        }

        commit_selected();

        if (edit_undo) {
            edit_undo();
        }

        tab_->level.signs.push_back(og::shared::level_sign{.x = 0, .y = 0, .text = ""});
        refresh();
        list_->set_selected_row(static_cast<int>(tab_->level.signs.size()) - 1);
        load_selected();
    };

    auto erase = editor_profiles::make_button("Delete", 70);
    erase->set_position({94, 196});
    erase->clicked = [this] {
        if (tab_ == nullptr || editing_ < 0 || editing_ >= static_cast<int>(tab_->level.signs.size())) {
            return;
        }

        if (edit_undo) {
            edit_undo();
        }

        tab_->level.signs.erase(tab_->level.signs.begin() + editing_);
        editing_ = -1;
        refresh();
        load_selected();
    };

    window_->add_child(editor_profiles::make_label("X:", {186, 199}, 20));
    x_edit_ = editor_profiles::make_edit({208, 196}, {56, 22});
    window_->add_child(editor_profiles::make_label("Y:", {276, 199}, 20));
    y_edit_ = editor_profiles::make_edit({298, 196}, {56, 22});

    text_edit_ = std::make_shared<gui_ml_text_edit_ctrl>();
    text_edit_->set_profile(editor_profiles::edit());
    text_edit_->set_scrollbar_profile(editor_profiles::scrollbar());
    text_edit_->set_position({16, 228});
    text_edit_->set_size({dialog_size.x - 32, dialog_size.y - 278});

    auto close_button = editor_profiles::make_button("Close", 90);
    close_button->set_position({dialog_size.x - 106, dialog_size.y - 38});
    close_button->clicked = [this] { close(); };

    window_->add_child(list_);
    window_->add_child(add);
    window_->add_child(erase);
    window_->add_child(x_edit_);
    window_->add_child(y_edit_);
    window_->add_child(text_edit_);
    window_->add_child(close_button);

    root->add_child(window_);
}

void editor_sign_dialog::show(editor_tab *tab) {
    tab_ = tab;
    editing_ = -1;

    refresh();
    load_selected();

    editor_profiles::centre_window(*window_, dialog_size);

    if (open_modal) {
        open_modal(*window_);
    }
}

auto editor_sign_dialog::is_visible() const -> bool {
    return window_ && window_->is_visible();
}

void editor_sign_dialog::refresh() {
    list_->clear_rows();

    if (tab_ == nullptr) {
        return;
    }

    auto id = 0;
    for (const auto &sign : tab_->level.signs) {
        list_->add_row(id, std::format("{}\t({}, {})", id + 1, sign.x, sign.y));
        ++id;
    }
}

void editor_sign_dialog::load_selected() {
    editing_ = list_->get_selected_index();

    if (tab_ == nullptr || editing_ < 0 || editing_ >= static_cast<int>(tab_->level.signs.size())) {
        editing_ = -1;
        x_edit_->set_text("");
        y_edit_->set_text("");
        text_edit_->set_text("");

        return;
    }

    const auto &sign = tab_->level.signs[static_cast<size_t>(editing_)];

    x_edit_->set_text(std::to_string(sign.x));
    y_edit_->set_text(std::to_string(sign.y));
    text_edit_->set_text(sign.text);
}

void editor_sign_dialog::commit_selected() {
    if (tab_ == nullptr || editing_ < 0 || editing_ >= static_cast<int>(tab_->level.signs.size())) {
        return;
    }

    auto &sign = tab_->level.signs[static_cast<size_t>(editing_)];
    const auto x = og::shared::to_int(x_edit_->get_text());
    const auto y = og::shared::to_int(y_edit_->get_text());
    const auto text = text_edit_->get_text();

    if (sign.x == x && sign.y == y && sign.text == text) {
        return;
    }

    if (edit_undo) {
        edit_undo();
    }

    sign.x = x;
    sign.y = y;
    sign.text = text;

    refresh();
}

void editor_sign_dialog::close() {
    if (!is_visible()) {
        return;
    }

    commit_selected();

    for (const auto &field : {x_edit_, y_edit_}) {
        field->blur();
    }
    text_edit_->blur();

    tab_ = nullptr;
    editing_ = -1;

    if (close_modal) {
        close_modal(*window_);
    }
}
