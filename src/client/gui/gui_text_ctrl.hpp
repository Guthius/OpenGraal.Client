#pragma once

#include "gui_control.hpp"

class gui_text_ctrl : public gui_control {
  public:
    [[nodiscard]] auto get_text() const -> std::string { return text_; }

    void set_text(const std::string &text);
    void set_fit_to_text(const bool fit) { fit_to_text_ = fit; }

    void draw() const override;

  private:
    std::string text_;
    bool fit_to_text_ = false;
};
