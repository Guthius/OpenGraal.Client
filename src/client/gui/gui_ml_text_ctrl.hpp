#pragma once

#include "gui_control.hpp"

#include <string>
#include <vector>

class gui_ml_text_ctrl : public gui_control {
  public:
    [[nodiscard]] auto get_text() const -> std::string;
    [[nodiscard]] auto get_lines() const -> const std::vector<std::string> & { return lines_; }
    [[nodiscard]] auto get_line_count() const -> size_t { return lines_.size(); }
    [[nodiscard]] auto is_word_wrap() const -> bool { return word_wrap_; }
    [[nodiscard]] auto is_parse_tags() const -> bool { return parse_tags_; }

    virtual void set_text(const std::string &text);
    virtual void set_lines(std::vector<std::string> lines);
    void add_text(const std::string &text);
    void set_word_wrap(const bool word_wrap) { word_wrap_ = word_wrap; }
    void set_parse_tags(const bool parse_tags) { parse_tags_ = parse_tags; }

    void set_line_height(const float height) { line_height_ = height; }

    void scroll_to_bottom();
    void scroll_to_top() { scroll_ = 0; }

    void draw() const override;

  protected:
    [[nodiscard]] auto wrap() const -> std::vector<std::string>;
    [[nodiscard]] auto line_height() const -> float;

    std::vector<std::string> lines_;
    size_t scroll_ = 0;

  private:
    bool word_wrap_ = true;
    bool parse_tags_ = true;
    float line_height_ = 0.0f;
};
