#pragma once

#include "../gui/gui_button_ctrl.hpp"
#include "../gui/gui_check_box_ctrl.hpp"
#include "../gui/gui_frame_set_ctrl.hpp"
#include "../gui/gui_radio_ctrl.hpp"
#include "../gui/gui_tab_ctrl.hpp"
#include "../gui/gui_text_ctrl.hpp"
#include "../gui/gui_window_ctrl.hpp"
#include "editor_canvas.hpp"
#include "editor_file_browser.hpp"
#include "editor_link_dialogs.hpp"
#include "editor_palette.hpp"
#include "editor_script_dialog.hpp"
#include "editor_sign_dialog.hpp"
#include "editor_state.hpp"

#include <net/message.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class editor_screen {
  public:
    using exit_handler = std::function<void()>;
    using release_handler = std::function<void(const std::string &name)>;
    using save_handler = std::function<void(const std::string &name, const std::string &content)>;

    auto attach(const std::shared_ptr<gui_control> &root) -> void;
    auto update(float dt) -> void;

    [[nodiscard]] auto is_visible() const -> bool;
    auto show() -> void;
    auto hide() -> void;

    auto open_level(const std::string &name, og::shared::level_data level, bool converted) -> void;
    auto new_level() -> void;
    auto close_current_tab() -> void;

    [[nodiscard]] auto has_unsaved() const -> bool;
    [[nodiscard]] auto active_tab() -> editor_tab *;

    auto request_exit() -> void;

    auto set_status(const std::string &text) -> void;

    auto handle_save_result(const std::string &name, og::net::edit_status status, const std::string &detail) -> void;
    auto handle_directory_listing(int category, const std::vector<std::string> &names) -> void;

    void on_exit(exit_handler handler) { exit_handler_ = std::move(handler); }
    void on_release_level(release_handler handler) { release_handler_ = std::move(handler); }
    void on_save_level(save_handler handler) { save_handler_ = std::move(handler); }
    void on_list_directory(std::function<void(int category)> handler) { list_directory_handler_ = std::move(handler); }

    void on_open_level_request(std::function<void(const std::string &name)> handler) { open_level_handler_ = std::move(handler); }

  private:
    auto build() -> void;
    auto build_side_panel() -> void;
    auto layout(float width, float height) -> void;
    auto layout_panel_widgets() -> void;
    auto select_tab(int id) -> void;
    auto refresh_tabs() -> void;
    auto close_tab_at(size_t index) -> void;
    auto show_confirm(const std::string &message, std::function<void()> confirmed) -> void;
    auto close_confirm() -> void;
    auto refresh_overlay() -> void;

    auto build_dialogs(const std::shared_ptr<gui_control> &root) -> void;
    auto push_modal(gui_window_ctrl &window) -> void;
    auto pop_modal(gui_window_ctrl &window) -> void;
    auto open_script_editor(int npc_index) -> void;
    auto open_warp_dialog() -> void;
    auto save_as() -> void;
    auto enter_mode(editor_mode mode) -> void;
    static auto push_undo(editor_tab &tab, undo_entry entry) -> void;
    auto undo() -> void;
    auto cut() -> void;
    auto copy() -> void;
    auto paste() -> void;
    auto erase_selection() -> void;
    auto save_active_tab() -> void;
    auto stamp_brush(editor_tab &tab, Vector2 level_tile) -> void;
    auto drop_floating(editor_tab &tab) -> void;
    [[nodiscard]] auto selection_status(const editor_tab &tab) const -> std::string;
    auto on_canvas_pressed(int button, Vector2 level_tile) -> void;
    auto on_canvas_moved(Vector2 level_tile) -> void;
    auto on_canvas_released(int button, Vector2 level_tile) -> void;

    std::shared_ptr<gui_control> body_;
    std::shared_ptr<gui_control> overlay_;
    std::shared_ptr<gui_control> toolbar_;
    std::shared_ptr<gui_control> divider_;
    std::shared_ptr<gui_tab_ctrl> tabs_ctrl_;
    std::shared_ptr<gui_frame_set_ctrl> frame_set_;
    std::shared_ptr<editor_canvas> canvas_;
    std::shared_ptr<gui_control> side_panel_;
    std::shared_ptr<gui_control> status_strip_;
    float panel_width_ = 220.0f;
    std::shared_ptr<editor_palette> palette_;
    std::shared_ptr<gui_radio_ctrl> tiles_radio_;
    std::shared_ptr<gui_radio_ctrl> npcs_radio_;
    std::shared_ptr<gui_text_ctrl> npc_hint_;
    std::shared_ptr<gui_check_box_ctrl> grid_check_;
    std::shared_ptr<gui_check_box_ctrl> npcs_check_;
    std::shared_ptr<gui_text_ctrl> status_;
    std::shared_ptr<gui_control> swatch_;
    std::shared_ptr<gui_button_ctrl> play_;
    std::shared_ptr<gui_button_ctrl> undo_button_;
    std::shared_ptr<gui_button_ctrl> cut_button_;
    std::shared_ptr<gui_button_ctrl> copy_button_;
    std::shared_ptr<gui_button_ctrl> paste_button_;
    std::shared_ptr<gui_button_ctrl> delete_button_;
    std::shared_ptr<gui_button_ctrl> warp_button_;
    std::shared_ptr<gui_window_ctrl> confirm_window_;
    std::shared_ptr<gui_text_ctrl> confirm_text_;
    std::function<void()> confirm_action_;

    editor_mode mode_ = editor_mode::none;
    tile_clipboard brush_;
    tile_clipboard clipboard_;
    og::shared::level_npc npc_clipboard_;
    bool clipboard_is_npc_ = false;
    bool pasting_npc_ = false;
    tile_clipboard floating_;
    int floating_x_ = 0;
    int floating_y_ = 0;
    Vector2 move_grab_{};
    bool selecting_ = false;
    bool moving_selection_ = false;
    bool duplicating_ = false;
    bool painting_ = false;
    bool dragging_npc_ = false;
    bool duplicating_npc_ = false;
    bool npc_moved_ = false;
    Vector2 npc_grab_{};
    int sel_anchor_x_ = 0;
    int sel_anchor_y_ = 0;
    int sel_cursor_x_ = 0;
    int sel_cursor_y_ = 0;
    double last_canvas_click_ = 0.0;
    int last_click_x_ = -1;
    int last_click_y_ = -1;

    std::vector<editor_tab> tabs_;
    std::vector<int> tab_ids_;
    int active_ = -1;
    int next_tab_id_ = 1;

    editor_file_browser file_browser_;
    editor_script_dialog script_dialog_;
    editor_link_dialog link_dialog_;
    editor_link_list_dialog link_list_;
    editor_sign_dialog sign_dialog_;
    std::vector<gui_window_ctrl *> modal_stack_;

    exit_handler exit_handler_;
    release_handler release_handler_;
    save_handler save_handler_;
    std::function<void(int)> list_directory_handler_;
    std::function<void(const std::string &)> open_level_handler_;
    Vector2 screen_{};
};

auto get_editor_screen() -> editor_screen &;
