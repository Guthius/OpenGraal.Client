#pragma once

#include "../gui/gui_text_edit_ctrl.hpp"
#include "../gui/gui_text_list_ctrl.hpp"
#include "../gui/gui_window_ctrl.hpp"
#include "editor_state.hpp"

#include <functional>
#include <memory>
#include <string>

class editor_link_dialog {
  public:
    auto attach(const std::shared_ptr<gui_control> &root) -> void;

    void show(const og::shared::level_link &link, std::function<void(og::shared::level_link)> accepted);

    [[nodiscard]] auto is_visible() const -> bool;
    void set_destination(const std::string &name);

    std::function<void()> browse_level;
    std::function<void(gui_window_ctrl &)> open_modal;
    std::function<void(gui_window_ctrl &)> close_modal;

  private:
    void accept();
    void close();

    std::shared_ptr<gui_window_ctrl> window_;
    std::shared_ptr<gui_text_edit_ctrl> x_edit_;
    std::shared_ptr<gui_text_edit_ctrl> y_edit_;
    std::shared_ptr<gui_text_edit_ctrl> width_edit_;
    std::shared_ptr<gui_text_edit_ctrl> height_edit_;
    std::shared_ptr<gui_text_edit_ctrl> destination_edit_;
    std::shared_ptr<gui_text_edit_ctrl> new_x_edit_;
    std::shared_ptr<gui_text_edit_ctrl> new_y_edit_;
    std::function<void(og::shared::level_link)> accepted_;
};

class editor_link_list_dialog {
  public:
    auto attach(const std::shared_ptr<gui_control> &root) -> void;

    void show(editor_tab *tab);
    void refresh();

    [[nodiscard]] auto is_visible() const -> bool;

    std::function<void(int index)> edit_link;
    std::function<void(const std::string &destination)> load_level_requested;
    std::function<void()> delete_undo;
    std::function<void(gui_window_ctrl &)> open_modal;
    std::function<void(gui_window_ctrl &)> close_modal;

  private:
    void close();

    std::shared_ptr<gui_window_ctrl> window_;
    std::shared_ptr<gui_text_list_ctrl> list_;
    editor_tab *tab_ = nullptr;
};
