#pragma once

#include "gui_text_ctrl.hpp"
#include "../font_manager.hpp"

#include <algorithm>
#include <string>

class gui_text_edit_ctrl : public gui_text_ctrl {
  public:
    [[nodiscard]]
    auto is_password() const -> bool { return password_; }

    void set_password(const bool password) { password_ = password; }
    void set_selection(size_t start, size_t length);

    void update(float dt) override;
    void draw() const override;

  protected:
    void on_mouse_pressed(int button, Vector2 pt) override;
    void on_mouse_move(Vector2 pt) override;
    void on_mouse_released(int button, Vector2 pt) override;

  private:
    static constexpr float k_text_padding = 4.0f;
    static constexpr float k_min_caret_width = 1.0f;

    bool password_{false};
    bool focused_{false};
    bool selecting_{false};
    size_t caret_index_{0};
    size_t selection_anchor_{0};
    float scroll_offset_{0.0f};
    Color background_color_{60, 90, 160, 160};
    Color caret_color_{240, 240, 240, 255};

    [[nodiscard]] auto get_display_text() const -> std::string;
    [[nodiscard]] auto get_selection_start() const -> size_t;
    [[nodiscard]] auto get_selection_end() const -> size_t;
    [[nodiscard]] auto get_selection_length() const -> size_t;

    [[nodiscard]] static auto text_width(const Font &font, const std::string &text) -> float;
    [[nodiscard]] static auto text_width_to_index(const Font &font, const std::string &text, size_t index) -> float;

    [[nodiscard]] auto index_from_x(float local_x) -> size_t {
        const auto profile = get_profile();
        if (!profile) {
            return 0;
        }

        const auto font = load_font(profile->get_font(), profile->get_font_size());
        const auto text = get_display_text();
        caret_index_ = std::min(caret_index_, text.size());
        selection_anchor_ = std::min(selection_anchor_, text.size());
        if (!IsFontValid(font) || text.empty()) {
            return 0;
        }

        const auto x = local_x - k_text_padding + scroll_offset_;
        if (x <= 0.0f) {
            return 0;
        }

        float accum = 0.0f;
        for (size_t i = 0; i < text.size(); ++i) {
            const auto char_width = text_width(font, text.substr(i, 1));
            if (accum + char_width * 0.5f >= x) {
                return i;
            }
            accum += char_width;
        }

        return text.size();
    }

    void delete_selection() {
        const auto start = get_selection_start();
        const auto end = get_selection_end();
        if (end <= start) {
            return;
        }

        auto text = get_text();
        text.erase(start, end - start);
        set_text(text);

        caret_index_ = start;
        selection_anchor_ = start;
    }

    void insert_text(const std::string &insert) {
        if (insert.empty()) {
            return;
        }

        if (get_selection_length() > 0) {
            delete_selection();
        }

        auto text = get_text();
        caret_index_ = std::min(caret_index_, text.size());
        text.insert(caret_index_, insert);
        caret_index_ += insert.size();
        selection_anchor_ = caret_index_;
        set_text(text);
    }

    void handle_keyboard_input() {
        auto text = get_text();

        const auto shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        if (IsKeyPressed(KEY_LEFT)) {
            if (caret_index_ > 0) {
                caret_index_--;
            }
            if (!shift) {
                selection_anchor_ = caret_index_;
            }
        }

        if (IsKeyPressed(KEY_RIGHT)) {
            if (caret_index_ < text.size()) {
                caret_index_++;
            }
            if (!shift) {
                selection_anchor_ = caret_index_;
            }
        }

        if (IsKeyPressed(KEY_HOME)) {
            caret_index_ = 0;
            if (!shift) {
                selection_anchor_ = caret_index_;
            }
        }

        if (IsKeyPressed(KEY_END)) {
            caret_index_ = text.size();
            if (!shift) {
                selection_anchor_ = caret_index_;
            }
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (get_selection_length() > 0) {
                delete_selection();
            } else if (caret_index_ > 0) {
                text.erase(caret_index_ - 1, 1);
                caret_index_--;
                selection_anchor_ = caret_index_;
                set_text(text);
            }
        }

        if (IsKeyPressed(KEY_DELETE)) {
            if (get_selection_length() > 0) {
                delete_selection();
            } else if (caret_index_ < text.size()) {
                text.erase(caret_index_, 1);
                selection_anchor_ = caret_index_;
                set_text(text);
            }
        }

        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key != 127) {
                const char c = static_cast<char>(key);
                insert_text(std::string(1, c));
            }
            key = GetCharPressed();
        }
    }

    void ensure_caret_visible() {
        const auto profile = get_profile();
        if (!profile) {
            return;
        }

        const auto font = load_font(profile->get_font(), profile->get_font_size());
        if (!IsFontValid(font)) {
            return;
        }

        const auto text = get_display_text();
        const auto total_width = text_width(font, text);
        const auto bounds = get_bounds();
        const auto view_width = std::max(0.0f, bounds.width - k_text_padding * 2.0f);

        if (total_width <= view_width) {
            scroll_offset_ = 0.0f;
            return;
        }

        const auto caret_x = text_width_to_index(font, text, caret_index_);
        if (caret_x - scroll_offset_ > view_width) {
            scroll_offset_ = caret_x - view_width;
        } else if (caret_x < scroll_offset_) {
            scroll_offset_ = caret_x;
        }

        const auto max_scroll = std::max(0.0f, total_width - view_width);
        scroll_offset_ = std::clamp(scroll_offset_, 0.0f, max_scroll);
    }

    void draw_selection(Color color, Font font, const std::string &text, Rectangle bounds, float padding) const {
        const auto start = get_selection_start();
        const auto end = get_selection_end();
        if (end <= start || !IsFontValid(font)) {
            return;
        }

        const auto start_x = text_width_to_index(font, text, start);
        const auto end_x = text_width_to_index(font, text, end);

        const auto draw_x = bounds.x + padding + start_x - scroll_offset_;
        const auto draw_w = end_x - start_x;

        if (draw_w <= 0.0f) {
            return;
        }

        DrawRectangleRec(
            Rectangle{
                draw_x,
                bounds.y + 1.0f,
                draw_w,
                bounds.height - 2.0f},
            color);
    }

    void draw_caret(Color color, Font font, const std::string &text, Rectangle bounds, float padding) const {
        if (!IsFontValid(font)) {
            return;
        }

        const auto caret_x = text_width_to_index(font, text, caret_index_);
        const auto draw_x = bounds.x + padding + caret_x - scroll_offset_;

        DrawRectangleRec(
            Rectangle{
                draw_x,
                bounds.y + 2.0f,
                k_min_caret_width,
                bounds.height - 4.0f},
            color);
    }
};
