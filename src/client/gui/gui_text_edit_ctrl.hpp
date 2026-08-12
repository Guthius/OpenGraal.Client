#pragma once

#include "gui_text_ctrl.hpp"

#include <functional>
#include <string>
#include <vector>

class gui_text_edit_ctrl : public gui_text_ctrl {
  public:
    std::function<void()> submitted;

    [[nodiscard]]
    auto is_password() const -> bool { return password_; }

    [[nodiscard]]
    auto is_dropdown_open() const -> bool { return dropdown_open_; }

    void set_password(const bool password) { password_ = password; }
    void set_max_length(const size_t length) { max_length_ = length; }
    void set_read_only(const bool read_only) { read_only_ = read_only; }
    void set_dropdown_rows(std::vector<std::string> rows);
    void set_selection(size_t start, size_t length);

    void update(float dt) override;
    void draw() const override;

  protected:
    void on_focus() override;
    void on_mouse_enter() override;
    void on_mouse_exit() override;
    void on_mouse_pressed(int button, Vector2 pt) override;
    void on_mouse_move(Vector2 pt) override;
    void on_mouse_released(int button, Vector2 pt) override;
    void on_blur() override;

  private:
    bool password_{false};
    bool read_only_{false};
    bool selecting_{false};
    bool dropdown_open_{false};
    size_t caret_index_{0};
    size_t selection_anchor_{0};
    size_t max_length_{0};
    float scroll_offset_{0.0f};
    float caret_timer_{0.0f};
    size_t last_caret_index_{0};
    std::vector<std::string> dropdown_rows_;

    [[nodiscard]] auto get_display_text() const -> std::string;
    [[nodiscard]] auto get_selection_start() const -> size_t;
    [[nodiscard]] auto get_selection_end() const -> size_t;
    [[nodiscard]] auto get_selection_length() const -> size_t;

    [[nodiscard]] static auto text_width(const Font &font, float size, const std::string &text) -> float;
    [[nodiscard]] static auto text_width_to_index(const Font &font, float size, const std::string &text, size_t index) -> float;

    [[nodiscard]] auto index_from_x(float local_x) -> size_t;

    [[nodiscard]] static auto band_height(const Font &font, float size, const std::string &text, float bounds_height) -> float;

    void delete_selection();
    void insert_text(const std::string &insert);
    void handle_keyboard_input();
    void open_dropdown();
    void close_dropdown();
    [[nodiscard]] auto dropdown_bounds() const -> Rectangle;
    [[nodiscard]] auto dropdown_row_at(Vector2 pt) const -> int;
    [[nodiscard]] auto arrow_width() const -> float;
    void ensure_caret_visible();
    void draw_selection(Color color, Font font, float size, const std::string &text, Rectangle bounds, float padding) const;
    void draw_caret(Color color, Font font, float size, const std::string &text, Rectangle bounds, float padding) const;
};
