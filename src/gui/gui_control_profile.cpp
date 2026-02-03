#include "gui_control_profile.hpp"

#include <algorithm>
#include <cmath>
#include <raymath.h>
#include <rlgl.h>

#include "../font_manager.hpp"
#include "../texture_manager.hpp"
#include "../utils.hpp"

namespace {
    auto with_alpha(Color color, const float alpha) -> Color {
        color.a = static_cast<unsigned char>(alpha * 255.0f);

        return color;
    }
}

void gui_control_profile::set_texture(const std::string &texture_filename) {
    texture_filename_ = texture_filename;
    texture_ = load_texture(texture_filename_);
    sprite_rects_ = find_sprite_rects(texture_);
}

void gui_control_profile::set_font(const std::string &font_filename) {
    font_filename_ = font_filename;
    font_ = load_font(font_filename_, font_size_);
}

void gui_control_profile::set_font_size(const float font_size) {
    font_size_ = font_size;
    font_ = load_font(font_filename_, font_size_);
}

void gui_control_profile::set_text_shadow(const bool text_shadow) {
    text_shadow_ = text_shadow;
}

void gui_control_profile::set_shadow_color(const Color color) {
    shadow_color_ = color;
}

void gui_control_profile::set_shadow_offset(const Vector2 offset) {
    shadow_offset_ = offset;
}

void gui_control_profile::set_transparency(const float transparency) {
    transparency_ = std::clamp(transparency, 0.0f, 1.0f);
}

auto gui_control_profile::get_sprite_rect(const int sprite) const -> Rectangle {
    if (sprite < 0 || sprite >= sprite_rects_.size()) {
        return {};
    }

    return sprite_rects_[sprite];
}

void gui_control_profile::draw_frame(const Rectangle &rect, const bool hot, const bool disabled) const {
    auto color_fill =
        disabled
            ? fill_color_na
        : hot
            ? fill_color_hl
            : fill_color;

    auto color_border =
        disabled
            ? border_color_na
        : hot
            ? border_color_hl
            : border_color;

    color_fill = with_alpha(color_fill, transparency_);
    color_border = with_alpha(color_border, transparency_);

    const auto x = static_cast<int>(rect.x);
    const auto y = static_cast<int>(rect.y);
    const auto width = static_cast<int>(rect.width);
    const auto height = static_cast<int>(rect.height);

    DrawRectangle(x, y, width, height, color_fill);

    const int thickness = std::max(0, border_thickness);
    if (thickness > 0) {
        DrawRectangle(x, y, width, thickness, color_border);
        DrawRectangle(x, y + height - thickness, width, thickness, color_border);
        DrawRectangle(x, y, thickness, height, color_border);
        DrawRectangle(x + width - thickness, y, thickness, height, color_border);
    }
}

void gui_control_profile::draw_text(const std::string &text, const Rectangle &rect, const alignment alignment) const {
    if (!IsFontValid(font_)) {
        return;
    }

    const auto [text_width, text_height] = MeasureTextEx(font_, text.c_str(), font_size_, 1.0f);

    const auto dest = [&]() -> Vector2 {
        switch (alignment) {
        default:
        case alignment::left:
            return {
                .x = std::round(rect.x),
                .y = std::round(rect.y + (rect.height - text_height) / 2)};
        case alignment::center:
            return {
                .x = std::round(rect.x + (rect.width - text_width) / 2),
                .y = std::round(rect.y + (rect.height - text_height) / 2)};
        case alignment::right:
            return {
                .x = std::round(rect.x + rect.width - text_width),
                .y = std::round(rect.y + (rect.height - text_height) / 2)};
        }
    }();

    if (text_shadow_) {
        DrawTextEx(font_, text.c_str(), dest + shadow_offset_, font_size_, 1.0f, shadow_color_);
    }

    DrawTextEx(font_, text.c_str(), dest, font_size_, 1.0f, with_alpha(font_color_, transparency_));
}

void gui_control_profile::draw_nine_patch(const Rectangle &rect, const nine_patch &patch) const {
    if (!IsTextureValid(texture_) || sprite_rects_.empty()) {
        return;
    }

    auto get_sprite_rect = [this](const int i) -> Rectangle {
        if (i < 0 || i >= static_cast<int>(sprite_rects_.size())) {
            return Rectangle{0, 0, 0, 0};
        }

        return sprite_rects_[static_cast<size_t>(i)];
    };

    const auto src_tl = get_sprite_rect(patch.tl);
    const auto src_t = get_sprite_rect(patch.t);
    const auto src_tr = get_sprite_rect(patch.tr);
    const auto src_l = get_sprite_rect(patch.l);
    const auto src_c = get_sprite_rect(patch.c);
    const auto src_r = get_sprite_rect(patch.r);
    const auto src_bl = get_sprite_rect(patch.bl);
    const auto src_b = get_sprite_rect(patch.b);
    const auto src_br = get_sprite_rect(patch.br);

    const auto alpha = static_cast<unsigned char>(transparency_ * 255.0f);

    rlSetTexture(texture_.id);
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, alpha);

    const auto texture_w = static_cast<float>(texture_.width);
    const auto texture_h = static_cast<float>(texture_.height);

    auto emit_quad = [texture_w, texture_h](const Rectangle src, const Rectangle dest) {
        if (src.width <= 0 || src.height <= 0)
            return;

        const auto u0 = src.x / texture_w;
        const auto v0 = src.y / texture_h;
        const auto u1 = (src.x + src.width) / texture_w;
        const auto v1 = (src.y + src.height) / texture_h;

        const auto x0 = dest.x;
        const auto y0 = dest.y;
        const auto x1 = x0 + dest.width;
        const auto y1 = y0 + dest.height;

        rlTexCoord2f(u0, v0);
        rlVertex2f(x0, y0);

        rlTexCoord2f(u0, v1);
        rlVertex2f(x0, y1);

        rlTexCoord2f(u1, v1);
        rlVertex2f(x1, y1);

        rlTexCoord2f(u1, v0);
        rlVertex2f(x1, y0);
    };

    const auto height = rect.height - src_t.height - src_b.height;

    const auto y0 = rect.y;
    const auto y1 = y0 + src_t.height;
    const auto y2 = rect.y + rect.height - src_b.height;

    emit_quad(src_tl, {rect.x, y0, src_tl.width, src_tl.height});
    emit_quad(src_t, {rect.x + src_tl.width, y0, rect.width - src_tl.width - src_tr.width, src_t.height});
    emit_quad(src_tr, {rect.x + rect.width - src_tr.width, y0, src_tr.width, src_tr.height});
    emit_quad(src_l, {rect.x, y1, src_l.width, height});
    emit_quad(src_c, {rect.x + src_l.width, y1, rect.width - src_l.width - src_r.width, height});
    emit_quad(src_r, {rect.x + rect.width - src_r.width, y1, src_r.width, height});
    emit_quad(src_bl, {rect.x, y2, src_bl.width, src_bl.height});
    emit_quad(src_b, {rect.x + src_l.width, y2, rect.width - src_bl.width - src_br.width, src_b.height});
    emit_quad(src_br, {rect.x + rect.width - src_br.width, y2, src_r.width, src_br.height});

    rlEnd();
    rlSetTexture(0);
}

void gui_control_profile::draw_sprite(const int sprite, const Vector2 position) const {
    if (!IsTextureValid(texture_) || sprite < 0 || sprite >= sprite_rects_.size()) {
        return;
    }

    DrawTextureRec(texture_, sprite_rects_[sprite], position, WHITE);
}
