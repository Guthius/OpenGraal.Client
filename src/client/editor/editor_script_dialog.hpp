#pragma once

#include "../gui/gui_ml_text_edit_ctrl.hpp"
#include "../gui/gui_text_edit_ctrl.hpp"
#include "../gui/gui_window_ctrl.hpp"

#include <functional>
#include <memory>
#include <string>

class editor_script_dialog {
  public:
    auto attach(const std::shared_ptr<gui_control> &root) -> void;

    void show(const std::string &image, const std::string &script,
        std::function<void(const std::string &image, const std::string &script)> committed);

    [[nodiscard]] auto is_visible() const -> bool;
    void set_image(const std::string &image);

    std::function<void()> browse_image;
    std::function<void(gui_window_ctrl &)> open_modal;
    std::function<void(gui_window_ctrl &)> close_modal;

  private:
    void run_test();
    void close();

    std::shared_ptr<gui_window_ctrl> window_;
    std::shared_ptr<gui_text_edit_ctrl> image_edit_;
    std::shared_ptr<gui_ml_text_edit_ctrl> script_edit_;
    std::shared_ptr<gui_text_edit_ctrl> error_edit_;
    std::function<void(const std::string &, const std::string &)> committed_;
};
