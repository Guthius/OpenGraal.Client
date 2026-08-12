#pragma once

#include "../gui/gui_text_edit_ctrl.hpp"
#include "../gui/gui_text_list_ctrl.hpp"
#include "../gui/gui_window_ctrl.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class editor_file_browser {
  public:
    auto attach(const std::shared_ptr<gui_control> &root) -> void;

    void show(int category, bool save_mode, const std::string &initial_name, std::function<void(const std::string &name)> chosen);

    void set_listing(int category, const std::vector<std::string> &names);

    [[nodiscard]] auto is_visible() const -> bool;

    std::function<void(int category)> request_listing;
    std::function<void(gui_window_ctrl &)> open_modal;
    std::function<void(gui_window_ctrl &)> close_modal;

  private:
    void accept();
    void close();

    std::shared_ptr<gui_window_ctrl> window_;
    std::shared_ptr<gui_text_list_ctrl> list_;
    std::shared_ptr<gui_text_edit_ctrl> name_edit_;
    std::function<void(const std::string &)> chosen_;
    int category_ = 0;
    bool save_mode_ = false;
};
