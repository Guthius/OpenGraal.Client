#pragma once

#include <cstdint>
#include <optional>
#include <raylib.h>
#include <string>
#include <vector>

#include "gui_utils.hpp"

enum class gui_border_style : uint8_t {
    none = 0,
    single = 1,
    rectangle = 2,
    lowered = 3,
    raised = 4,
};

class gui_control_profile {
    std::string name_;
    std::string texture_filename_;
    Texture2D texture_{};
    std::vector<Rectangle> sprite_rects_;
    std::string font_filename_;
    float font_size_ = 14;
    Font font_{};
    bool text_shadow_ = false;
    Color shadow_color_ = BLACK;
    Vector2 shadow_offset_ = {1, 1};
    float transparency_ = 1.0f;

  public:
    void set_texture(const std::string &texture_filename);
    void set_font(const std::string &font_filename);
    void set_font_size(float font_size);
    void set_text_shadow(bool text_shadow);
    void set_shadow_color(Color color);
    void set_shadow_offset(Vector2 offset);
    void set_transparency(float transparency);

    void set_name(std::string name) { name_ = std::move(name); }

    [[nodiscard]] auto get_name() const -> const std::string & { return name_; }
    [[nodiscard]] auto get_texture_filename() const -> const std::string & { return texture_filename_; }
    [[nodiscard]] auto get_texture() const -> Texture2D { return texture_; }
    [[nodiscard]] auto get_sprite_rect(int sprite) const -> Rectangle;
    [[nodiscard]] auto get_font() const -> const std::string & { return font_filename_; }
    [[nodiscard]] auto get_font_size() const -> float { return font_size_; }

    [[nodiscard]] auto measure_text(const std::string &text) const -> Vector2;

    void draw_frame(const Rectangle &rect, bool hot, bool disabled) const;
    void draw_text(const std::string &text, const Rectangle &rect, alignment alignment, std::optional<Color> color = std::nullopt) const;
    void draw_nine_patch(const Rectangle &rect, const nine_patch &patch) const;
    void draw_sprite(int sprite, Vector2 position) const;

    gui_border_style border_style{gui_border_style::none};
    Color border_color{255, 255, 255, 255};
    Color border_color_hl{255, 255, 0, 255};
    Color border_color_na{128, 128, 128, 255};
    int border_thickness{1};
    Color fill_color{50, 50, 50, 255};
    Color fill_color_hl{70, 70, 70, 255};
    Color fill_color_na{40, 40, 40, 255};
    Color font_color{255, 255, 255, 255};
    Color font_color_hl{255, 255, 0, 255};
    Color font_color_na{160, 160, 160, 255};
    Color font_color_sel{255, 255, 255, 255};
    Color font_color_link{120, 160, 255, 255};
    Color font_color_link_hl{160, 200, 255, 255};
    std::string sound_button_down;
    std::string sound_button_over;
    Color caret_color = {240, 240, 240, 255};
};
