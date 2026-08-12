#include "editor_screen.hpp"

#include "../constants.hpp"
#include "../dev_input.hpp"
#include "../gui/gui.hpp"
#include "../tileset_manager.hpp"
#include "editor_ops.hpp"
#include "editor_profiles.hpp"

#include <shared/config.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <raylib.h>

namespace {
    constexpr float toolbar_height = 34.0f;
    constexpr float toolbar_button_size = 26.0f;
    constexpr float divider_height = 2.0f;
    constexpr float tab_height = 24.0f;
    constexpr float status_height = 22.0f;
    constexpr Vector2 confirm_size{380, 140};

    editor_screen instance;

    auto atlas_cell_text(const og::shared::tile_id tile) -> std::string {
        return std::format("({}, {})", ((tile / 512) * 16) + (tile % 16), (tile % 512) / 16);
    }

    class swatch_ctrl final : public gui_control {
      public:
        std::function<const tileset *()> tiles;
        std::function<og::shared::tile_id()> tile;

        void draw() const override {
            const auto *set = tiles ? tiles() : nullptr;
            if (set == nullptr || !tile) {
                return;
            }

            const auto id = tile();
            const auto bounds = get_bounds();
            const int atlas_x = (((id / 512) * 16) + (id % 16)) * tile_size;
            const int atlas_y = ((id % 512) / 16) * tile_size;

            DrawTexturePro(set->get_texture(),
                {static_cast<float>(atlas_x), static_cast<float>(atlas_y), tile_size, tile_size},
                bounds, {0, 0}, 0.0f, WHITE);
            DrawRectangleLinesEx(bounds, 1, BLACK);
        }
    };
}

auto get_editor_screen() -> editor_screen & {
    return instance;
}

auto editor_screen::attach(const std::shared_ptr<gui_control> &root) -> void {
    build();

    root->add_child(body_);
    root->add_child(overlay_);
    root->add_child(confirm_window_);

    build_dialogs(root);

    layout(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
}

auto editor_screen::build_dialogs(const std::shared_ptr<gui_control> &root) -> void {
    const auto modal_open = [this](gui_window_ctrl &window) { push_modal(window); };
    const auto modal_close = [this](gui_window_ctrl &window) { pop_modal(window); };

    file_browser_.attach(root);
    file_browser_.open_modal = modal_open;
    file_browser_.close_modal = modal_close;
    file_browser_.request_listing = [this](const int category) {
        if (list_directory_handler_) {
            list_directory_handler_(category);
        }
    };

    script_dialog_.attach(root);
    script_dialog_.open_modal = modal_open;
    script_dialog_.close_modal = modal_close;
    script_dialog_.browse_image = [this] {
        file_browser_.show(static_cast<int>(og::shared::folder_category::file), false, "",
            [this](const std::string &name) { script_dialog_.set_image(name); });
    };

    link_dialog_.attach(root);
    link_dialog_.open_modal = modal_open;
    link_dialog_.close_modal = modal_close;
    link_dialog_.browse_level = [this] {
        file_browser_.show(static_cast<int>(og::shared::folder_category::level), false, "",
            [this](const std::string &name) { link_dialog_.set_destination(name); });
    };

    link_list_.attach(root);
    link_list_.open_modal = modal_open;
    link_list_.close_modal = modal_close;
    link_list_.edit_link = [this](const int index) {
        auto *tab = active_tab();
        if (tab == nullptr || index < 0 || index >= static_cast<int>(tab->level.links.size())) {
            return;
        }

        link_dialog_.show(tab->level.links[static_cast<size_t>(index)], [this, index](og::shared::level_link link) {
            auto *current = active_tab();
            if (current == nullptr || index >= static_cast<int>(current->level.links.size())) {
                return;
            }

            push_undo(*current, links_snapshot{.before = current->level.links});
            current->level.links[static_cast<size_t>(index)] = std::move(link);
            link_list_.refresh();
        });
    };
    link_list_.load_level_requested = [this](const std::string &destination) {
        if (open_level_handler_) {
            open_level_handler_(destination);
        }
    };
    link_list_.delete_undo = [this] {
        if (auto *tab = active_tab()) {
            push_undo(*tab, links_snapshot{.before = tab->level.links});
        }
    };

    sign_dialog_.attach(root);
    sign_dialog_.open_modal = modal_open;
    sign_dialog_.close_modal = modal_close;
    sign_dialog_.edit_undo = [this] {
        if (auto *tab = active_tab()) {
            push_undo(*tab, signs_snapshot{.before = tab->level.signs});
        }
    };
}

auto editor_screen::push_modal(gui_window_ctrl &window) -> void {
    if (modal_stack_.empty()) {
        body_->set_disabled(true);
    } else {
        modal_stack_.back()->set_disabled(true);
    }

    window.set_disabled(false);
    window.set_visible(true);

    modal_stack_.push_back(&window);

    refresh_overlay();
}

auto editor_screen::pop_modal(gui_window_ctrl &window) -> void {
    window.set_visible(false);

    std::erase(modal_stack_, &window);

    if (modal_stack_.empty()) {
        body_->set_disabled(false);
    } else {
        modal_stack_.back()->set_disabled(false);
    }

    refresh_overlay();
}

auto editor_screen::refresh_overlay() -> void {
    auto *top = confirm_window_ && confirm_window_->is_visible() ? confirm_window_.get()
                : !modal_stack_.empty()                          ? modal_stack_.back()
                                                                 : nullptr;

    overlay_->set_visible(top != nullptr);

    if (top != nullptr) {
        overlay_->bring_to_front();
        top->bring_to_front();
    }
}

auto editor_screen::build() -> void {
    body_ = std::make_shared<gui_control>();
    body_->set_profile(editor_profiles::body());
    body_->set_visible(false);

    overlay_ = std::make_shared<gui_control>();
    overlay_->set_profile(editor_profiles::overlay());
    overlay_->set_visible(false);

    toolbar_ = std::make_shared<gui_control>();
    toolbar_->set_profile(editor_profiles::toolbar());
    toolbar_->set_position({0, 0});

    divider_ = std::make_shared<gui_control>();
    divider_->set_profile(editor_profiles::divider());
    divider_->set_position({0, toolbar_height});

    struct entry {
        const char *icon;
        const char *hint;
        std::function<void()> action;
        bool group_end;
        std::shared_ptr<gui_button_ctrl> *slot = nullptr;
    };

    const auto entries = std::vector<entry>{
        {"icon_new.png", "New level", [this] { new_level(); }, false},
        {"icon_close.png", "Close level", [this] { close_current_tab(); }, true},
        {"icon_open.png", "Open level",
         [this] {
                file_browser_.show(static_cast<int>(og::shared::folder_category::level), false, "",
                    [this](const std::string &name) {
                        if (open_level_handler_) {
                            open_level_handler_(name);
                        }
                    });
            },
         false},
        {"icon_save.png", "Save level", [this] { save_active_tab(); }, false},
        {"icon_save_as.png", "Save level as", [this] { save_as(); }, true},
        {"icon_undo.png", "Undo", [this] { undo(); }, true, &undo_button_},
        {"icon_cut.png", "Cut", [this] { cut(); }, false, &cut_button_},
        {"icon_copy.png", "Copy", [this] { copy(); }, false, &copy_button_},
        {"icon_paste.png", "Paste", [this] { paste(); }, false, &paste_button_},
        {"icon_delete.png", "Delete", [this] { erase_selection(); }, true, &delete_button_},
        {"icon_warp.png", "Turn selection into warp field", [this] { open_warp_dialog(); }, false, &warp_button_},
        {"icon_links.png", "Edit links",
         [this] {
                if (auto *tab = active_tab()) {
                    link_list_.show(tab);
                }
            },
         false},
        {"icon_signs.png", "Edit signs",
         [this] {
                if (auto *tab = active_tab()) {
                    sign_dialog_.show(tab);
                }
            },
         false},
    };

    auto x = 6.0f;
    for (const auto &[icon, hint, action, group_end, slot] : entries) {
        auto button = editor_profiles::make_button("", toolbar_button_size);
        button->set_size({toolbar_button_size, toolbar_button_size});
        button->set_position({x, 4});
        button->set_icon_size(20, 20);
        button->get_icon()->draw_image(icon, {0, 0});
        button->set_hint(hint);
        button->clicked = action;

        if (slot != nullptr) {
            *slot = button;
        }

        toolbar_->add_child(button);
        x += toolbar_button_size + 3.0f + (group_end ? 12.0f : 0.0f);
    }

    play_ = editor_profiles::make_button("Play", 80);
    play_->set_size({80, toolbar_button_size});
    play_->set_icon_size(20, 20);
    play_->get_icon()->draw_image("icon_play.png", {0, 0});
    play_->set_hint("Leave the editor and play");
    play_->clicked = [this] { request_exit(); };
    toolbar_->add_child(play_);

    tabs_ctrl_ = std::make_shared<gui_tab_ctrl>();
    tabs_ctrl_->set_profile(editor_profiles::tab());
    tabs_ctrl_->set_position({0, toolbar_height + divider_height});
    tabs_ctrl_->selected = [this](const int id, const std::string &, int) { select_tab(id); };

    canvas_ = std::make_shared<editor_canvas>();
    canvas_->set_profile(editor_profiles::canvas());
    canvas_->set_position({0, toolbar_height + tab_height});
    canvas_->hovered = [this](const int x, const int y, const og::shared::tile_id tile) {
        if (mode_ == editor_mode::npc_placement) {
            set_status(std::format("npc -> ({}, {})", x, y));

            return;
        }

        const auto *tab = active_tab();
        if (tab != nullptr && tab->selection.active && (tab->selection.width > 0 || tab->selection.height > 0)) {
            set_status(selection_status(*tab));

            return;
        }

        set_status(std::format("({}, {}) - tile {}", x, y, atlas_cell_text(tile)));
    };
    canvas_->canvas_pressed = [this](const int button, const Vector2 tile) { on_canvas_pressed(button, tile); };
    canvas_->canvas_moved = [this](const Vector2 tile) { on_canvas_moved(tile); };
    canvas_->canvas_released = [this](const int button, const Vector2 tile) { on_canvas_released(button, tile); };

    build_side_panel();

    frame_set_ = std::make_shared<gui_frame_set_ctrl>();
    frame_set_->set_column_count(2);
    frame_set_->set_border_width(4);
    frame_set_->set_min_extent({160, 40});
    frame_set_->add_child(canvas_);
    frame_set_->add_child(side_panel_);
    frame_set_->resized = [this] {
        panel_width_ = side_panel_->get_size().x;
        layout_panel_widgets();
    };

    status_strip_ = std::make_shared<gui_control>();
    status_strip_->set_profile(editor_profiles::body());

    status_ = std::make_shared<gui_text_ctrl>();
    status_->set_profile(editor_profiles::status());

    auto swatch = std::make_shared<swatch_ctrl>();
    swatch->tiles = [this]() -> const tileset * {
        return palette_->tileset_source ? palette_->tileset_source() : nullptr;
    };
    swatch->tile = [this] { return palette_->background_tile(); };
    swatch_ = swatch;

    body_->add_child(toolbar_);
    body_->add_child(divider_);
    body_->add_child(tabs_ctrl_);
    body_->add_child(frame_set_);
    body_->add_child(status_strip_);
    body_->add_child(status_);
    body_->add_child(swatch_);

    confirm_window_ = std::make_shared<gui_window_ctrl>();
    confirm_window_->set_profile(editor_profiles::window());
    confirm_window_->set_size(confirm_size);
    confirm_window_->set_text("Level editor");
    confirm_window_->set_visible(false);

    confirm_text_ = std::make_shared<gui_text_ctrl>();
    confirm_text_->set_profile(editor_profiles::dialog_label());
    confirm_text_->set_position({20, 44});
    confirm_text_->set_size({confirm_size.x - 40, 20});

    auto ok = editor_profiles::make_button("OK", 90);
    ok->set_position({(confirm_size.x / 2) - 98, confirm_size.y - 40});
    ok->clicked = [this] {
        const auto action = std::move(confirm_action_);
        close_confirm();

        if (action) {
            action();
        }
    };

    auto cancel = editor_profiles::make_button("Cancel", 90);
    cancel->set_position({(confirm_size.x / 2) + 8, confirm_size.y - 40});
    cancel->clicked = [this] { close_confirm(); };

    confirm_window_->add_child(confirm_text_);
    confirm_window_->add_child(ok);
    confirm_window_->add_child(cancel);
}

auto editor_screen::build_side_panel() -> void {
    side_panel_ = std::make_shared<gui_control>();
    side_panel_->set_profile(editor_profiles::panel());

    tiles_radio_ = std::make_shared<gui_radio_ctrl>();
    tiles_radio_->set_profile(editor_profiles::radio());
    tiles_radio_->set_position({12, 8});
    tiles_radio_->set_size({panel_width_ - 24, 18});
    tiles_radio_->set_text("Select tile");
    tiles_radio_->set_group(1);
    tiles_radio_->set_checked(true);
    tiles_radio_->clicked = [this] { enter_mode(editor_mode::none); };

    npcs_radio_ = std::make_shared<gui_radio_ctrl>();
    npcs_radio_->set_profile(editor_profiles::radio());
    npcs_radio_->set_position({12, 30});
    npcs_radio_->set_size({panel_width_ - 24, 18});
    npcs_radio_->set_text("Predefined NPCs");
    npcs_radio_->set_group(1);
    npcs_radio_->clicked = [this] { enter_mode(editor_mode::npc_placement); };

    palette_ = std::make_shared<editor_palette>();
    palette_->set_profile(editor_profiles::palette());
    palette_->tileset_source = [this]() -> const tileset * {
        auto *tab = active_tab();

        return tab != nullptr ? tileset_for_level(tab->level.name) : tileset_for_level("");
    };
    palette_->brush_selected = [this](const tile_clipboard &brush) {
        brush_ = brush;
        enter_mode(editor_mode::tile_placement);
    };

    npc_hint_ = std::make_shared<gui_text_ctrl>();
    npc_hint_->set_profile(editor_profiles::label());
    npc_hint_->set_text("Click the level to place an NPC.");
    npc_hint_->set_visible(false);

    grid_check_ = std::make_shared<gui_check_box_ctrl>();
    grid_check_->set_profile(editor_profiles::check());
    grid_check_->set_size({panel_width_ - 24, 18});
    grid_check_->set_text("Show terrain grid");
    grid_check_->clicked = [this] { canvas_->set_show_grid(grid_check_->is_checked()); };

    npcs_check_ = std::make_shared<gui_check_box_ctrl>();
    npcs_check_->set_profile(editor_profiles::check());
    npcs_check_->set_size({panel_width_ - 24, 18});
    npcs_check_->set_text("Hide npcs");
    npcs_check_->clicked = [this] { canvas_->set_hide_npcs(npcs_check_->is_checked()); };

    side_panel_->add_child(tiles_radio_);
    side_panel_->add_child(npcs_radio_);
    side_panel_->add_child(palette_);
    side_panel_->add_child(npc_hint_);
    side_panel_->add_child(grid_check_);
    side_panel_->add_child(npcs_check_);
}

auto editor_screen::layout(const float width, const float height) -> void {
    if (width == screen_.x && height == screen_.y) {
        return;
    }

    screen_ = {width, height};

    body_->set_position({0, 0});
    body_->set_size({width, height});

    overlay_->set_position({0, 0});
    overlay_->set_size({width, height});

    toolbar_->set_size({width, toolbar_height});
    play_->set_position({width - 86, 4});

    divider_->set_size({width, divider_height});
    tabs_ctrl_->set_size({width, tab_height});

    const auto content_top = toolbar_height + divider_height + tab_height;

    frame_set_->set_position({0, content_top});
    frame_set_->set_size({width, height - content_top});
    frame_set_->set_column_offset(1, width - panel_width_ - 4.0f - (2 * gui_frame_set_ctrl::bevel_width));

    layout_panel_widgets();

    confirm_window_->set_position({
        std::round((width - confirm_size.x) / 2.0f),
        std::round((height - confirm_size.y) / 2.0f),
    });
}

auto editor_screen::layout_panel_widgets() -> void {
    const auto bevel = gui_frame_set_ctrl::bevel_width;
    const auto width = screen_.x;
    const auto height = screen_.y;
    const auto content_top = toolbar_height + divider_height + tab_height;
    const auto panel_height = height - content_top - (2 * bevel);
    const auto panel_x = width - panel_width_ - bevel;

    palette_->set_position({8, 56});
    palette_->set_size({std::max(40.0f, panel_width_ - 16), panel_height - status_height - 56 - 60});

    npc_hint_->set_position({12, 60});
    npc_hint_->set_size({std::max(40.0f, panel_width_ - 24), 18});

    tiles_radio_->set_size({std::max(40.0f, panel_width_ - 24), 18});
    npcs_radio_->set_size({std::max(40.0f, panel_width_ - 24), 18});

    grid_check_->set_position({12, panel_height - status_height - 50});
    grid_check_->set_size({std::max(40.0f, panel_width_ - 24), 18});
    npcs_check_->set_position({12, panel_height - status_height - 28});
    npcs_check_->set_size({std::max(40.0f, panel_width_ - 24), 18});

    status_strip_->set_position({panel_x, height - bevel - status_height});
    status_strip_->set_size({panel_width_, status_height});

    status_->set_position({panel_x + 4, height - bevel - status_height + 2});
    status_->set_size({std::max(40.0f, panel_width_ - 32), 18});

    swatch_->set_position({width - bevel - 20, height - bevel - status_height + 3});
    swatch_->set_size({16, 16});
}

auto editor_screen::update(const float /*dt*/) -> void {
    layout(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));

    if (!is_visible()) {
        return;
    }

    canvas_->set_tab(active_tab());

    {
        auto *tab = active_tab();
        const auto tile_selection = tab != nullptr && tab->selection.active &&
                                    tab->selection.width > 0 && tab->selection.height > 0;
        const auto npc_selected = tab != nullptr && tab->selected_npc >= 0;

        undo_button_->set_disabled(tab == nullptr || tab->undo.empty());
        cut_button_->set_disabled(!tile_selection && !npc_selected);
        copy_button_->set_disabled(!tile_selection && !npc_selected);
        paste_button_->set_disabled(tab == nullptr || (!clipboard_is_npc_ && clipboard_.tiles.empty()));
        delete_button_->set_disabled(!tile_selection && !npc_selected);
        warp_button_->set_disabled(!tile_selection);
    }

    if (dev_input::key_pressed(KEY_F11)) {
        request_exit();
    }

    if (dev_input::key_pressed(KEY_ESCAPE) && mode_ != editor_mode::none) {
        brush_ = {};
        enter_mode(editor_mode::none);
    }

    const auto keys_free = mode_ == editor_mode::none && modal_stack_.empty() && !confirm_window_->is_visible() && !gui_has_focus();

    if (keys_free) {
        if (dev_input::key_pressed(KEY_DELETE)) {
            erase_selection();
        }

        if (dev_input::key_down(KEY_LEFT_CONTROL) || dev_input::key_down(KEY_RIGHT_CONTROL)) {
            if (dev_input::key_pressed(KEY_X)) {
                cut();
            }

            if (dev_input::key_pressed(KEY_C)) {
                copy();
            }

            if (dev_input::key_pressed(KEY_V)) {
                paste();
            }

            if (dev_input::key_pressed(KEY_Z)) {
                undo();
            }
        }
    }
}

auto editor_screen::is_visible() const -> bool {
    return body_ && body_->is_visible();
}

auto editor_screen::show() -> void {
    if (!body_) {
        return;
    }

    body_->set_visible(true);
    body_->set_disabled(false);
}

auto editor_screen::hide() -> void {
    if (!body_) {
        return;
    }

    close_confirm();

    tabs_.clear();
    tab_ids_.clear();
    active_ = -1;
    refresh_tabs();
    canvas_->set_tab(nullptr);

    body_->set_visible(false);
}

auto editor_screen::open_level(const std::string &name, og::shared::level_data level, const bool converted) -> void {
    for (size_t i = 0; i < tabs_.size(); ++i) {
        if (tabs_[i].server_name == name) {
            active_ = static_cast<int>(i);
            refresh_tabs();

            return;
        }
    }

    auto tab = editor_tab{
        .title = name,
        .server_name = name,
        .level = std::move(level),
        .converted = converted,
    };

    tabs_.push_back(std::move(tab));
    tab_ids_.push_back(next_tab_id_++);
    active_ = static_cast<int>(tabs_.size()) - 1;

    refresh_tabs();

    if (converted) {
        set_status(std::format("{} was opened as .graal and will save as .nw", name));
    }
}

auto editor_screen::new_level() -> void {
    auto number = 1;
    while (std::ranges::any_of(tabs_, [&](const editor_tab &tab) {
        return tab.title == std::format("new{}.nw", number);
    })) {
        ++number;
    }

    auto level = og::shared::level_data{};
    level.name = std::format("new{}.nw", number);
    level.version = "GLEVNW01";
    level.layers.push_back(og::shared::level_layer{
        .index = 0,
        .tiles = std::vector<og::shared::tile_id>(og::shared::level_tile_count, editor_default_tile),
    });

    tabs_.push_back(editor_tab{
        .title = level.name,
        .server_name = "",
        .level = std::move(level),
    });
    tab_ids_.push_back(next_tab_id_++);
    active_ = static_cast<int>(tabs_.size()) - 1;

    refresh_tabs();
}

auto editor_screen::close_current_tab() -> void {
    if (active_ < 0) {
        return;
    }

    const auto index = static_cast<size_t>(active_);

    if (tabs_[index].dirty) {
        show_confirm(std::format("Close {}? Unsaved changes will be lost.", tabs_[index].title),
            [this, index] { close_tab_at(index); });

        return;
    }

    close_tab_at(index);
}

auto editor_screen::close_tab_at(const size_t index) -> void {
    if (index >= tabs_.size()) {
        return;
    }

    if (!tabs_[index].server_name.empty() && release_handler_) {
        release_handler_(tabs_[index].server_name);
    }

    tabs_.erase(tabs_.begin() + static_cast<ptrdiff_t>(index));
    tab_ids_.erase(tab_ids_.begin() + static_cast<ptrdiff_t>(index));

    if (tabs_.empty()) {
        active_ = -1;
    } else if (active_ >= static_cast<int>(tabs_.size())) {
        active_ = static_cast<int>(tabs_.size()) - 1;
    }

    refresh_tabs();
}

auto editor_screen::has_unsaved() const -> bool {
    return std::ranges::any_of(tabs_, [](const editor_tab &tab) { return tab.dirty; });
}

auto editor_screen::active_tab() -> editor_tab * {
    return active_ >= 0 && active_ < static_cast<int>(tabs_.size()) ? &tabs_[static_cast<size_t>(active_)] : nullptr;
}

auto editor_screen::request_exit() -> void {
    const auto leave = [this] {
        if (exit_handler_) {
            exit_handler_();
        }
    };

    if (has_unsaved()) {
        show_confirm("Leave the editor? Unsaved changes will be lost.", leave);

        return;
    }

    leave();
}

auto editor_screen::set_status(const std::string &text) -> void {
    if (status_) {
        status_->set_text(text);
    }
}

auto editor_screen::select_tab(const int id) -> void {
    for (size_t i = 0; i < tab_ids_.size(); ++i) {
        if (tab_ids_[i] == id) {
            active_ = static_cast<int>(i);
            canvas_->set_tab(active_tab());

            return;
        }
    }
}

auto editor_screen::refresh_tabs() -> void {
    tabs_ctrl_->clear_rows();

    for (size_t i = 0; i < tabs_.size(); ++i) {
        tabs_ctrl_->add_row(tab_ids_[i], tabs_[i].title);
    }

    if (active_ >= 0) {
        tabs_ctrl_->set_selected_row(tab_ids_[static_cast<size_t>(active_)]);
    }

    canvas_->set_tab(active_tab());
}

auto editor_screen::handle_directory_listing(const int category, const std::vector<std::string> &names) -> void {
    file_browser_.set_listing(category, names);
}

auto editor_screen::open_script_editor(const int npc_index) -> void {
    auto *tab = active_tab();
    if (tab == nullptr || npc_index < 0 || npc_index >= static_cast<int>(tab->level.npcs.size())) {
        return;
    }

    const auto &npc = tab->level.npcs[static_cast<size_t>(npc_index)];

    script_dialog_.show(npc.image, npc.script, [this, npc_index](const std::string &image, const std::string &script) {
        auto *current = active_tab();
        if (current == nullptr || npc_index >= static_cast<int>(current->level.npcs.size())) {
            return;
        }

        auto &edited = current->level.npcs[static_cast<size_t>(npc_index)];
        if (edited.image == image && edited.script == script) {
            return;
        }

        push_undo(*current, npcs_snapshot{.before = current->level.npcs});
        edited.image = image;
        edited.script = script;
    });
}

auto editor_screen::open_warp_dialog() -> void {
    auto *tab = active_tab();
    if (tab == nullptr) {
        return;
    }

    auto prefilled = og::shared::level_link{};

    if (tab->selection.active && tab->selection.width > 0 && tab->selection.height > 0) {
        prefilled.x = tab->selection.x;
        prefilled.y = tab->selection.y;
        prefilled.width = tab->selection.width;
        prefilled.height = tab->selection.height;
    }

    link_dialog_.show(prefilled, [this](og::shared::level_link link) {
        auto *current = active_tab();
        if (current == nullptr) {
            return;
        }

        push_undo(*current, links_snapshot{.before = current->level.links});
        current->level.links.push_back(std::move(link));
    });
}

auto editor_screen::save_as() -> void {
    auto *tab = active_tab();
    if (tab == nullptr) {
        return;
    }

    file_browser_.show(static_cast<int>(og::shared::folder_category::level), true, tab->title,
        [this](const std::string &name) {
            auto *current = active_tab();
            if (current == nullptr) {
                return;
            }

            current->server_name = name;
            current->title = name;
            refresh_tabs();
            save_active_tab();
        });
}

auto editor_screen::enter_mode(const editor_mode mode) -> void {
    if (mode != editor_mode::none && mode_ == editor_mode::none) {
        if (auto *tab = active_tab()) {
            tab->selection.active = false;
        }
    }

    mode_ = mode;
    pasting_npc_ = false;
    canvas_->set_mode(mode);
    canvas_->set_brush(mode == editor_mode::tile_placement ? &brush_ : nullptr);

    const auto npcs = mode == editor_mode::npc_placement;

    npcs_radio_->set_checked(npcs);
    tiles_radio_->set_checked(!npcs);
    palette_->set_visible(!npcs);
    npc_hint_->set_visible(npcs);
}

auto editor_screen::push_undo(editor_tab &tab, undo_entry entry) -> void {
    tab.undo.push_back(std::move(entry));

    if (tab.undo.size() > 50) {
        tab.undo.pop_front();
    }

    tab.dirty = true;
}

auto editor_screen::undo() -> void {
    auto *tab = active_tab();
    if (tab == nullptr || tab->undo.empty()) {
        return;
    }

    auto entry = std::move(tab->undo.back());
    tab->undo.pop_back();

    if (const auto *tiles = std::get_if<tile_patch>(&entry)) {
        editor_ops::paste(tab->level, tiles->x, tiles->y,
            tile_clipboard{.width = tiles->width, .height = tiles->height, .tiles = tiles->before});
    } else if (const auto *npcs = std::get_if<npcs_snapshot>(&entry)) {
        tab->level.npcs = npcs->before;
        tab->selected_npc = -1;
    } else if (const auto *links = std::get_if<links_snapshot>(&entry)) {
        tab->level.links = links->before;
    } else if (const auto *signs = std::get_if<signs_snapshot>(&entry)) {
        tab->level.signs = signs->before;
    }
}

auto editor_screen::copy() -> void {
    auto *tab = active_tab();
    if (tab == nullptr) {
        return;
    }

    if (tab->selection.active && tab->selection.width > 0 && tab->selection.height > 0) {
        clipboard_ = editor_ops::copy_rect(
            tab->level, tab->selection.x, tab->selection.y, tab->selection.width, tab->selection.height);
        clipboard_is_npc_ = false;

        return;
    }

    if (tab->selected_npc >= 0 && tab->selected_npc < static_cast<int>(tab->level.npcs.size())) {
        npc_clipboard_ = tab->level.npcs[static_cast<size_t>(tab->selected_npc)];
        clipboard_is_npc_ = true;
    }
}

auto editor_screen::erase_selection() -> void {
    auto *tab = active_tab();
    if (tab == nullptr) {
        return;
    }

    if (tab->selection.active && tab->selection.width > 0 && tab->selection.height > 0) {
        push_undo(*tab, editor_ops::snapshot(tab->level));
        editor_ops::fill_rect(tab->level, tab->selection.x, tab->selection.y,
            tab->selection.width, tab->selection.height, palette_->background_tile());

        return;
    }

    if (tab->selected_npc >= 0 && tab->selected_npc < static_cast<int>(tab->level.npcs.size())) {
        push_undo(*tab, npcs_snapshot{.before = tab->level.npcs});
        tab->level.npcs.erase(tab->level.npcs.begin() + tab->selected_npc);
        tab->selected_npc = -1;
    }
}

auto editor_screen::cut() -> void {
    copy();
    erase_selection();
}

auto editor_screen::paste() -> void {
    if (active_tab() == nullptr) {
        return;
    }

    if (clipboard_is_npc_) {
        enter_mode(editor_mode::npc_placement);

        pasting_npc_ = true;

        return;
    }

    if (clipboard_.tiles.empty()) {
        return;
    }

    brush_ = clipboard_;

    enter_mode(editor_mode::tile_placement);
}

auto editor_screen::save_active_tab() -> void {
    auto *tab = active_tab();
    if (tab == nullptr || !save_handler_) {
        return;
    }

    if (tab->server_name.empty()) {
        set_status("Use Save as to name a new level.");

        return;
    }

    save_handler_(tab->server_name, og::shared::save_level_text(tab->level));
    set_status(std::format("Saving {}...", tab->server_name));
}

auto editor_screen::handle_save_result(const std::string &name, const og::net::edit_status status, const std::string &detail) -> void {
    if (status == og::net::edit_status::ok) {
        for (auto &tab : tabs_) {
            if (tab.server_name == name) {
                tab.dirty = false;
            }
        }

        set_status(std::format("Saved {}", name));

        return;
    }

    const auto reason = detail.empty()
                            ? std::string(og::net::describe(status))
                            : std::format("{} {}", og::net::describe(status), detail);

    set_status(std::format("Could not save {}: {}", name, reason));
}

auto editor_screen::stamp_brush(editor_tab &tab, const Vector2 level_tile) -> void {
    if (brush_.tiles.empty()) {
        return;
    }

    editor_ops::paste(tab.level,
        static_cast<int>(std::floor(level_tile.x)),
        static_cast<int>(std::floor(level_tile.y)),
        brush_);

    tab.dirty = true;
}

auto editor_screen::drop_floating(editor_tab &tab) -> void {
    if (!moving_selection_) {
        return;
    }

    if (duplicating_) {
        push_undo(tab, editor_ops::snapshot(tab.level));

        duplicating_ = false;
    }

    editor_ops::paste(tab.level, floating_x_, floating_y_, floating_);

    tab.selection.x = floating_x_;
    tab.selection.y = floating_y_;
    sel_anchor_x_ = floating_x_;
    sel_anchor_y_ = floating_y_;
    sel_cursor_x_ = floating_x_ + tab.selection.width;
    sel_cursor_y_ = floating_y_ + tab.selection.height;

    moving_selection_ = false;
    floating_ = {};
    canvas_->set_floating(nullptr, 0, 0);
}

auto editor_screen::selection_status(const editor_tab & /*tab*/) const -> std::string {
    return std::format("({}, {}) -> ({}, {}) = ({}, {})",
        sel_anchor_x_, sel_anchor_y_, sel_cursor_x_, sel_cursor_y_,
        std::abs(sel_cursor_x_ - sel_anchor_x_), std::abs(sel_cursor_y_ - sel_anchor_y_));
}

auto editor_screen::on_canvas_pressed(const int button, const Vector2 level_tile) -> void {
    auto *tab = active_tab();
    if (tab == nullptr) {
        return;
    }

    const auto tile_x = static_cast<int>(std::floor(level_tile.x));
    const auto tile_y = static_cast<int>(std::floor(level_tile.y));

    if (button == MOUSE_BUTTON_RIGHT) {
        if (mode_ != editor_mode::none) {
            return;
        }

        const auto &selection = tab->selection;
        if (selection.active && selection.width > 0 && selection.height > 0 &&
            tile_x >= selection.x && tile_x < selection.x + selection.width &&
            tile_y >= selection.y && tile_y < selection.y + selection.height) {
            moving_selection_ = true;
            duplicating_ = true;

            floating_ = editor_ops::copy_rect(tab->level, selection.x, selection.y, selection.width, selection.height);
            floating_x_ = selection.x;
            floating_y_ = selection.y;
            move_grab_ = {level_tile.x - static_cast<float>(selection.x), level_tile.y - static_cast<float>(selection.y)};
            canvas_->set_floating(&floating_, floating_x_, floating_y_);

            return;
        }

        if (const auto index = canvas_->npc_at(level_tile); index >= 0) {
            tab->selection.active = false;
            tab->selected_npc = index;

            dragging_npc_ = true;
            duplicating_npc_ = true;
            npc_moved_ = false;
            npc_grab_ = {
                level_tile.x - tab->level.npcs[static_cast<size_t>(index)].x,
                level_tile.y - tab->level.npcs[static_cast<size_t>(index)].y,
            };

            return;
        }

        push_undo(*tab, editor_ops::snapshot(tab->level));
        editor_ops::flood_fill(tab->level, tile_x, tile_y, palette_->background_tile());

        return;
    }

    if (button != MOUSE_BUTTON_LEFT) {
        return;
    }

    const auto double_click = GetTime() - last_canvas_click_ < 0.35 && tile_x == last_click_x_ && tile_y == last_click_y_;

    last_canvas_click_ = GetTime();
    last_click_x_ = tile_x;
    last_click_y_ = tile_y;

    if (mode_ == editor_mode::tile_placement) {
        painting_ = true;
        push_undo(*tab, editor_ops::snapshot(tab->level));
        stamp_brush(*tab, level_tile);

        return;
    }

    if (mode_ == editor_mode::npc_placement) {
        push_undo(*tab, npcs_snapshot{.before = tab->level.npcs});

        const auto pasted = pasting_npc_;

        auto npc = pasted ? npc_clipboard_ : og::shared::level_npc{};

        npc.x = static_cast<float>(tile_x);
        npc.y = static_cast<float>(tile_y);

        tab->level.npcs.push_back(std::move(npc));
        tab->selected_npc = static_cast<int>(tab->level.npcs.size()) - 1;

        enter_mode(editor_mode::none);

        if (!pasted) {
            open_script_editor(tab->selected_npc);
        }

        return;
    }

    if (const auto index = canvas_->npc_at(level_tile); index >= 0) {
        tab->selection.active = false;
        tab->selected_npc = index;

        if (double_click) {
            open_script_editor(index);

            return;
        }

        dragging_npc_ = true;
        npc_moved_ = false;
        npc_grab_ = {
            level_tile.x - tab->level.npcs[static_cast<size_t>(index)].x,
            level_tile.y - tab->level.npcs[static_cast<size_t>(index)].y,
        };

        return;
    }

    tab->selected_npc = -1;

    if (double_click) {
        if (const auto *tiles = tab->level.tiles(); tiles != nullptr &&
                                                    tile_x >= 0 && tile_x < og::shared::level_width &&
                                                    tile_y >= 0 && tile_y < og::shared::level_height) {
            palette_->set_background_tile((*tiles)[(tile_y * og::shared::level_width) + tile_x]);
        }

        return;
    }

    const auto &selection = tab->selection;
    if (selection.active && selection.width > 0 && selection.height > 0 &&
        tile_x >= selection.x && tile_x < selection.x + selection.width &&
        tile_y >= selection.y && tile_y < selection.y + selection.height) {
        moving_selection_ = true;

        push_undo(*tab, editor_ops::snapshot(tab->level));
        floating_ = editor_ops::copy_rect(tab->level, selection.x, selection.y, selection.width, selection.height);
        editor_ops::fill_rect(tab->level, selection.x, selection.y, selection.width, selection.height, palette_->background_tile());

        floating_x_ = selection.x;
        floating_y_ = selection.y;
        move_grab_ = {level_tile.x - static_cast<float>(selection.x), level_tile.y - static_cast<float>(selection.y)};
        canvas_->set_floating(&floating_, floating_x_, floating_y_);

        return;
    }

    selecting_ = true;
    sel_anchor_x_ = sel_cursor_x_ = tile_x;
    sel_anchor_y_ = sel_cursor_y_ = tile_y;

    tab->selection = tile_selection{.active = true, .x = tile_x, .y = tile_y, .width = 0, .height = 0};
}

auto editor_screen::on_canvas_moved(const Vector2 level_tile) -> void {
    auto *tab = active_tab();
    if (tab == nullptr) {
        return;
    }

    const auto tile_x = static_cast<int>(std::floor(level_tile.x));
    const auto tile_y = static_cast<int>(std::floor(level_tile.y));

    if (painting_ && mode_ == editor_mode::tile_placement) {
        stamp_brush(*tab, level_tile);

        return;
    }

    if (selecting_) {
        sel_cursor_x_ = std::clamp(tile_x, 0, og::shared::level_width);
        sel_cursor_y_ = std::clamp(tile_y, 0, og::shared::level_height);

        tab->selection.x = std::min(sel_anchor_x_, sel_cursor_x_);
        tab->selection.y = std::min(sel_anchor_y_, sel_cursor_y_);
        tab->selection.width = std::abs(sel_cursor_x_ - sel_anchor_x_);
        tab->selection.height = std::abs(sel_cursor_y_ - sel_anchor_y_);

        set_status(selection_status(*tab));

        return;
    }

    if (moving_selection_) {
        floating_x_ = static_cast<int>(std::floor(level_tile.x - move_grab_.x));
        floating_y_ = static_cast<int>(std::floor(level_tile.y - move_grab_.y));
        canvas_->set_floating(&floating_, floating_x_, floating_y_);

        return;
    }

    if (dragging_npc_ && tab->selected_npc >= 0) {
        if (!npc_moved_) {
            npc_moved_ = true;
            push_undo(*tab, npcs_snapshot{.before = tab->level.npcs});

            if (duplicating_npc_) {
                tab->level.npcs.push_back(tab->level.npcs[static_cast<size_t>(tab->selected_npc)]);
                tab->selected_npc = static_cast<int>(tab->level.npcs.size()) - 1;
            }
        }

        auto &npc = tab->level.npcs[static_cast<size_t>(tab->selected_npc)];

        npc.x = static_cast<float>(std::clamp(
            static_cast<int>(std::floor(level_tile.x - npc_grab_.x + 0.5f)), 0, og::shared::level_width - 1));
        npc.y = static_cast<float>(std::clamp(
            static_cast<int>(std::floor(level_tile.y - npc_grab_.y + 0.5f)), 0, og::shared::level_height - 1));
    }
}

auto editor_screen::on_canvas_released(const int button, const Vector2 /*level_tile*/) -> void {
    auto *tab = active_tab();
    if (tab == nullptr) {
        return;
    }

    if (button == MOUSE_BUTTON_RIGHT) {
        if (moving_selection_ && duplicating_) {
            drop_floating(*tab);
        }

        if (dragging_npc_) {
            dragging_npc_ = false;
            duplicating_npc_ = false;
            npc_moved_ = false;
        }

        return;
    }

    if (button != MOUSE_BUTTON_LEFT) {
        return;
    }

    if (painting_) {
        painting_ = false;
        brush_ = {};
        enter_mode(editor_mode::none);

        return;
    }

    if (selecting_) {
        selecting_ = false;

        if (tab->selection.width == 0 || tab->selection.height == 0) {
            tab->selection.active = false;
        }

        return;
    }

    if (moving_selection_) {
        drop_floating(*tab);

        return;
    }

    dragging_npc_ = false;
    duplicating_npc_ = false;
    npc_moved_ = false;
}

auto editor_screen::show_confirm(const std::string &message, std::function<void()> confirmed) -> void {
    confirm_action_ = std::move(confirmed);
    confirm_text_->set_text(message);

    confirm_window_->set_visible(true);

    body_->set_disabled(true);

    refresh_overlay();
}

auto editor_screen::close_confirm() -> void {
    if (!confirm_window_ || !confirm_window_->is_visible()) {
        return;
    }

    confirm_window_->set_visible(false);
    confirm_action_ = nullptr;

    body_->set_disabled(false);

    refresh_overlay();
}
