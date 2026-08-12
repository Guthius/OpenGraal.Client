#pragma once

#include "../gui/gui_ml_text_edit_ctrl.hpp"
#include "../gui/gui_text_edit_ctrl.hpp"
#include "../gui/gui_text_list_ctrl.hpp"
#include "../gui/gui_window_ctrl.hpp"
#include "editor_state.hpp"

#include <functional>
#include <memory>

class editor_sign_dialog {
  public:
    auto attach(const std::shared_ptr<gui_control> &root) -> void;

    void show(editor_tab *tab);

    [[nodiscard]] auto is_visible() const -> bool;

    std::function<void()> edit_undo;
    std::function<void(gui_window_ctrl &)> open_modal;
    std::function<void(gui_window_ctrl &)> close_modal;

  private:
    void refresh();
    void load_selected();
    void commit_selected();
    void close();

    std::shared_ptr<gui_window_ctrl> window_;
    std::shared_ptr<gui_text_list_ctrl> list_;
    std::shared_ptr<gui_text_edit_ctrl> x_edit_;
    std::shared_ptr<gui_text_edit_ctrl> y_edit_;
    std::shared_ptr<gui_ml_text_edit_ctrl> text_edit_;
    editor_tab *tab_ = nullptr;
    int editing_ = -1;
};
