#pragma once

#include "gui_control.hpp"

#include <functional>
#include <string>
#include <vector>

class gui_text_list_ctrl : public gui_control {
  public:
    struct row {
        int id = 0;
        std::string text;
    };

    using select_handler = std::function<void(int id, const std::string &text, int index)>;

    [[nodiscard]] auto get_rows() const -> const std::vector<row> & { return rows_; }

    [[nodiscard]] auto get_selected_row() const -> int;
    [[nodiscard]] auto get_selected_text() const -> std::string;
    [[nodiscard]] auto get_selected_index() const -> int { return selected_; }

    void clear_rows();
    void add_row(int id, const std::string &text);
    void remove_row(int id);

    void set_selected_row(int id);
    void set_text_profile(const std::shared_ptr<gui_control_profile> &profile) { text_profile_ = profile; }
    void set_columns(std::vector<float> columns) { columns_ = std::move(columns); }

    void set_line_height(const float height) { line_height_ = height; }

    void sort();

    select_handler selected;

    void update(float dt) override;
    void draw() const override;

  protected:
    void on_mouse_pressed(int button, Vector2 pt) override;

  private:
    [[nodiscard]] auto row_height() const -> float;
    [[nodiscard]] auto row_at(Vector2 pt) const -> int;
    [[nodiscard]] auto visible_rows() const -> int;

    [[nodiscard]] auto text_profile() const -> const std::shared_ptr<gui_control_profile> &;

    std::vector<row> rows_;
    std::vector<float> columns_;
    std::shared_ptr<gui_control_profile> text_profile_;
    float line_height_ = 20.0f;
    int selected_ = -1;
    int scroll_ = 0;
};
