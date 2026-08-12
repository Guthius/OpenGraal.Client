#pragma once

#include "gui_ml_text_ctrl.hpp"
#include "gui_scrollbar_ctrl.hpp"

#include <cstddef>
#include <vector>

class gui_ml_text_edit_ctrl final : public gui_ml_text_ctrl {
  public:
    gui_ml_text_edit_ctrl();

    [[nodiscard]] auto get_caret_line() const -> size_t { return caret_line_; }
    [[nodiscard]] auto get_caret_column() const -> size_t { return caret_column_; }
    [[nodiscard]] auto is_syntax_highlighting() const -> bool { return syntax_highlighting_; }
    [[nodiscard]] auto is_auto_indenting() const -> bool { return auto_indenting_; }
    [[nodiscard]] auto get_tab_spaces() const -> int { return tab_spaces_; }

    void set_syntax_highlighting(const bool on) { syntax_highlighting_ = on; }
    void set_auto_indenting(const bool on) { auto_indenting_ = on; }
    void set_tab_spaces(const int spaces) { tab_spaces_ = spaces; }
    void set_caret(size_t line, size_t column);
    void set_scrollbar_profile(const std::shared_ptr<gui_control_profile> &profile);

    void set_text(const std::string &text) override;
    void set_lines(std::vector<std::string> lines) override;

    void update(float dt) override;
    void draw() const override;

  protected:
    void on_mouse_pressed(int button, Vector2 pt) override;
    void on_mouse_move(Vector2 pt) override;
    void on_mouse_released(int button, Vector2 pt) override;

  private:
    [[nodiscard]] auto current_line() const -> const std::string &;

    struct span {
        std::string text;
        Color color;
    };

    struct caret_position {
        size_t line, column;

        constexpr auto operator<=>(const caret_position &) const = default;
    };

    [[nodiscard]] auto spans_of(const std::string &line) const -> std::vector<span>;
    void draw_line(const std::string &line, Vector2 at) const;

    [[nodiscard]] auto text_area() const -> Rectangle;
    [[nodiscard]] auto content_width() const -> float;
    [[nodiscard]] auto position_at(Vector2 pt) const -> caret_position;
    [[nodiscard]] auto has_selection() const -> bool;
    [[nodiscard]] auto selection_range() const -> std::pair<caret_position, caret_position>;

    void clear_selection();
    void erase_selection();
    void layout_scrollbars();

    void insert_text(const std::string &text);
    void insert_newline();
    void erase_before_caret();
    void erase_at_caret();
    void move_caret(int lines, int columns);
    void ensure_caret_visible();
    void handle_keyboard_input();

    std::shared_ptr<gui_scrollbar_ctrl> v_bar_;
    std::shared_ptr<gui_scrollbar_ctrl> h_bar_;
    size_t caret_line_ = 0;
    size_t caret_column_ = 0;
    size_t anchor_line_ = 0;
    size_t anchor_column_ = 0;
    float scroll_x_ = 0.0f;
    bool selecting_ = false;
    float blink_ = 0.0f;
    int tab_spaces_ = 0;
    bool syntax_highlighting_ = false;
    bool auto_indenting_ = false;
};
