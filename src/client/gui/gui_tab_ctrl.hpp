#pragma once

#include "gui_control.hpp"

#include <functional>
#include <string>
#include <vector>

class gui_tab_ctrl : public gui_control {
  public:
    struct row {
        int id = 0;
        std::string text;
    };

    using select_handler = std::function<void(int id, const std::string &text, int index)>;

    [[nodiscard]] auto get_rows() const -> const std::vector<row> & { return rows_; }
    [[nodiscard]] auto get_selected_row() const -> int;

    void clear_rows();
    void add_row(int id, const std::string &text);

    void set_selected_row(int id);

    select_handler selected;
    select_handler deselected;

    void draw() const override;

  protected:
    void on_mouse_pressed(int button, Vector2 pt) override;

  private:
    [[nodiscard]] auto width_of(const row &row) const -> float;
    [[nodiscard]] auto row_at(Vector2 pt) const -> int;

    std::vector<row> rows_;
    int selected_ = -1;
};
