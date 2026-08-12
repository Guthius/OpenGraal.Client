#pragma once

#include "gui/gui_text_edit_ctrl.hpp"

#include <functional>
#include <memory>
#include <string>

class chat_bar {
  public:
    using submit_handler = std::function<void(const std::string &text)>;

    void attach(const std::shared_ptr<gui_control> &root);
    void on_submit(submit_handler handler) { submit_handler_ = std::move(handler); }

    [[nodiscard]] auto is_open() const -> bool { return open_; }

    [[nodiscard]]
    auto get_input() const -> const std::shared_ptr<gui_text_edit_ctrl> & { return input_; }

    void open();
    void close();
    void toggle();

    auto update() -> bool;

    void layout(float screen_width, float screen_height) const;

  private:
    std::shared_ptr<gui_text_edit_ctrl> input_;
    submit_handler submit_handler_;
    bool open_ = false;
};

[[nodiscard]] auto get_chat_bar() -> chat_bar &;
