#include "editor_link_dialogs.hpp"

#include "../gui/gui_ml_text_ctrl.hpp"
#include "editor_profiles.hpp"

#include <shared/text.hpp>

#include <format>

namespace {
    constexpr Vector2 link_size{360, 330};
    constexpr Vector2 list_size{460, 320};

}

auto editor_link_dialog::attach(const std::shared_ptr<gui_control> &root) -> void {
    window_ = std::make_shared<gui_window_ctrl>();
    window_->set_profile(editor_profiles::window());
    window_->set_size(link_size);
    window_->set_text("Edit link");
    window_->set_visible(false);

    auto blurb = std::make_shared<gui_ml_text_ctrl>();
    blurb->set_profile(editor_profiles::dialog_label());
    blurb->set_position({16, 34});
    blurb->set_size({link_size.x - 32, 54});
    blurb->set_text(
        "Specify the warp field and the level where the player should be transported to. "
        "New X/Y accept expressions like playerx.");
    window_->add_child(blurb);

    window_->add_child(editor_profiles::make_label("X:", {16, 100}, 20));
    x_edit_ = editor_profiles::make_edit({38, 98}, {50, 22});
    window_->add_child(editor_profiles::make_label("Y:", {100, 100}, 20));
    y_edit_ = editor_profiles::make_edit({122, 98}, {50, 22});
    window_->add_child(editor_profiles::make_label("W:", {184, 100}, 24));
    width_edit_ = editor_profiles::make_edit({210, 98}, {50, 22});
    window_->add_child(editor_profiles::make_label("H:", {272, 100}, 20));
    height_edit_ = editor_profiles::make_edit({294, 98}, {50, 22});

    window_->add_child(editor_profiles::make_label("Link to level:", {16, 136}, 100));
    destination_edit_ = editor_profiles::make_edit({16, 158}, {link_size.x - 130, 24});

    auto browse = editor_profiles::make_button("Browse", 90);
    browse->set_position({link_size.x - 106, 159});
    browse->clicked = [this] {
        if (browse_level) {
            browse_level();
        }
    };

    window_->add_child(editor_profiles::make_label("New X:", {16, 200}, 60));
    new_x_edit_ = editor_profiles::make_edit({80, 198}, {180, 24});
    window_->add_child(editor_profiles::make_label("New Y:", {16, 232}, 60));
    new_y_edit_ = editor_profiles::make_edit({80, 230}, {180, 24});

    auto ok = editor_profiles::make_button("OK", 90);
    ok->set_position({(link_size.x / 2) - 98, link_size.y - 40});
    ok->clicked = [this] { accept(); };

    auto cancel = editor_profiles::make_button("Cancel", 90);
    cancel->set_position({(link_size.x / 2) + 8, link_size.y - 40});
    cancel->clicked = [this] { close(); };

    window_->add_child(x_edit_);
    window_->add_child(y_edit_);
    window_->add_child(width_edit_);
    window_->add_child(height_edit_);
    window_->add_child(destination_edit_);
    window_->add_child(browse);
    window_->add_child(new_x_edit_);
    window_->add_child(new_y_edit_);
    window_->add_child(ok);
    window_->add_child(cancel);

    root->add_child(window_);
}

void editor_link_dialog::show(const og::shared::level_link &link, std::function<void(og::shared::level_link)> accepted) {
    accepted_ = std::move(accepted);

    x_edit_->set_text(std::to_string(link.x));
    y_edit_->set_text(std::to_string(link.y));
    width_edit_->set_text(std::to_string(link.width));
    height_edit_->set_text(std::to_string(link.height));
    destination_edit_->set_text(link.destination);
    new_x_edit_->set_text(link.destination_x.empty() ? "playerx" : link.destination_x);
    new_y_edit_->set_text(link.destination_y.empty() ? "playery" : link.destination_y);

    editor_profiles::centre_window(*window_, link_size);

    if (open_modal) {
        open_modal(*window_);
    }

    destination_edit_->focus();
}

auto editor_link_dialog::is_visible() const -> bool {
    return window_ && window_->is_visible();
}

void editor_link_dialog::set_destination(const std::string &name) {
    if (destination_edit_) {
        destination_edit_->set_text(name);
    }
}

void editor_link_dialog::accept() {
    if (destination_edit_->get_text().empty()) {
        return;
    }

    auto link = og::shared::level_link{
        .destination = destination_edit_->get_text(),
        .x = og::shared::to_int(x_edit_->get_text()),
        .y = og::shared::to_int(y_edit_->get_text()),
        .width = og::shared::to_int(width_edit_->get_text()),
        .height = og::shared::to_int(height_edit_->get_text()),
        .destination_x = new_x_edit_->get_text(),
        .destination_y = new_y_edit_->get_text(),
    };

    const auto accepted = std::move(accepted_);
    close();

    if (accepted) {
        accepted(std::move(link));
    }
}

void editor_link_dialog::close() {
    if (!is_visible()) {
        return;
    }

    for (const auto &field : {x_edit_, y_edit_, width_edit_, height_edit_, destination_edit_, new_x_edit_, new_y_edit_}) {
        field->blur();
    }

    accepted_ = nullptr;

    if (close_modal) {
        close_modal(*window_);
    }
}

auto editor_link_list_dialog::attach(const std::shared_ptr<gui_control> &root) -> void {
    window_ = std::make_shared<gui_window_ctrl>();
    window_->set_profile(editor_profiles::window());
    window_->set_size(list_size);
    window_->set_text("Link list");
    window_->set_visible(false);

    window_->add_child(editor_profiles::make_label("Select a link to edit:", {16, 34}, 200));

    list_ = std::make_shared<gui_text_list_ctrl>();
    list_->set_profile(editor_profiles::list());
    list_->set_text_profile(editor_profiles::list_text());
    list_->set_position({16, 56});
    list_->set_size({list_size.x - 32, list_size.y - 110});
    list_->set_columns({4, 170, 210, 250, 290, 330, 380});

    auto edit = editor_profiles::make_button("Edit", 80);
    edit->set_position({16, list_size.y - 38});
    edit->clicked = [this] {
        if (const auto index = list_->get_selected_index(); index >= 0 && edit_link) {
            edit_link(index);
        }
    };

    auto load = editor_profiles::make_button("Load", 80);
    load->set_position({104, list_size.y - 38});
    load->clicked = [this] {
        const auto index = list_->get_selected_index();
        if (tab_ != nullptr && index >= 0 && index < static_cast<int>(tab_->level.links.size()) && load_level_requested) {
            const auto destination = tab_->level.links[static_cast<size_t>(index)].destination;
            close();
            load_level_requested(destination);
        }
    };

    auto erase = editor_profiles::make_button("Delete", 80);
    erase->set_position({192, list_size.y - 38});
    erase->clicked = [this] {
        const auto index = list_->get_selected_index();
        if (tab_ != nullptr && index >= 0 && index < static_cast<int>(tab_->level.links.size())) {
            if (delete_undo) {
                delete_undo();
            }

            tab_->level.links.erase(tab_->level.links.begin() + index);
            refresh();
        }
    };

    auto close_button = editor_profiles::make_button("Close", 80);
    close_button->set_position({list_size.x - 96, list_size.y - 38});
    close_button->clicked = [this] { close(); };

    window_->add_child(list_);
    window_->add_child(edit);
    window_->add_child(load);
    window_->add_child(erase);
    window_->add_child(close_button);

    root->add_child(window_);
}

void editor_link_list_dialog::show(editor_tab *tab) {
    tab_ = tab;

    refresh();
    editor_profiles::centre_window(*window_, list_size);

    if (open_modal) {
        open_modal(*window_);
    }
}

void editor_link_list_dialog::refresh() {
    list_->clear_rows();

    if (tab_ == nullptr) {
        return;
    }

    auto id = 0;
    for (const auto &link : tab_->level.links) {
        list_->add_row(id++, std::format("{}\t{}\t{}\t{}\t{}\t{}\t{}",
                                 link.destination, link.x, link.y, link.width, link.height,
                                 link.destination_x, link.destination_y));
    }
}

auto editor_link_list_dialog::is_visible() const -> bool {
    return window_ && window_->is_visible();
}

void editor_link_list_dialog::close() {
    if (!is_visible()) {
        return;
    }

    tab_ = nullptr;

    if (close_modal) {
        close_modal(*window_);
    }
}
