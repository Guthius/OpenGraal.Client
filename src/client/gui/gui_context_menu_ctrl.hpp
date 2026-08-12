#pragma once

#include <string>
#include <vector>

#include "gui_control.hpp"

class gui_context_menu_ctrl final : public gui_control {
  public:
    struct row {
        int id = 0;
        std::string text;
    };

    gui_context_menu_ctrl() { set_visible(false); }

    [[nodiscard]] auto get_rows() const -> const std::vector<row> & { return rows_; }
    [[nodiscard]] auto get_selected_row() const -> int { return selected_; }

    void add_row(int id, std::string text);
    void clear_rows();
    void open(Vector2 position);
    void close() { set_visible(false); }

    void draw() const override;

  protected:
    void on_mouse_released(int button, Vector2 pt) override;

  private:
    [[nodiscard]] auto row_height() const -> float;

    std::vector<row> rows_;
    int selected_ = -1;
};
