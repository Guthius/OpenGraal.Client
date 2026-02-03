#pragma once

#include "gui_control.hpp"

class gui_text_ctrl : public gui_control {
  public:
    [[nodiscard]] auto get_text() const -> std::string { return text_; }

    void set_text(const std::string &text) { text_ = text; }

  protected:
    void draw() const override;

  private:
    std::string text_;
};
