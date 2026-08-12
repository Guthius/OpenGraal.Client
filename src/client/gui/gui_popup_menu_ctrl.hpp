#pragma once

#include "gui_text_ctrl.hpp"

#include <functional>
#include <string>
#include <vector>

class gui_popup_menu_ctrl : public gui_text_ctrl {
  public:
    struct row {
        int id = 0;
        std::string text;
    };

    using select_handler = std::function<void(int id, const std::string &text, int index)>;

    [[nodiscard]] auto get_rows() const -> const std::vector<row> & { return rows_; }
    [[nodiscard]] auto get_selected_id() const -> int;
    [[nodiscard]] auto get_selected_text() const -> std::string;
    [[nodiscard]] auto get_selected_index() const -> int { return selected_; }
    [[nodiscard]] auto is_open() const -> bool { return open_; }

    void clear_rows();
    void add_row(int id, const std::string &text);
    void insert_row(int index, int id, const std::string &text);
    void remove_row(int id);
    void clear_selection();

    void set_selected_index(int index);
    void set_selected_id(int id);

    void open();
    void close();

    void set_text_profile(const std::shared_ptr<gui_control_profile> &profile) { text_profile_ = profile; }
    void set_max_popup_height(const float height) { max_popup_height_ = height; }

    select_handler selected;

    void draw() const override;

  protected:
    void on_mouse_pressed(int button, Vector2 pt) override;
    void on_blur() override;

  private:
    [[nodiscard]] auto row_height() const -> float;
    [[nodiscard]] auto popup_height() const -> float;
    [[nodiscard]] auto row_at(Vector2 pt) const -> int;
    [[nodiscard]] auto text_profile() const -> const std::shared_ptr<gui_control_profile> &;

    void select(int index);

    std::vector<row> rows_;
    std::shared_ptr<gui_control_profile> text_profile_;
    float max_popup_height_ = 200.0f;
    int selected_ = -1;
    bool open_ = false;
};
