#pragma once

#include "../animation.hpp"
#include "gui_control.hpp"

class gui_show_img_ctrl : public gui_control {
  public:
    [[nodiscard]] auto get_image() const -> const std::string & { return image_; }
    [[nodiscard]] auto get_ani() const -> const std::string & { return ani_; }
    [[nodiscard]] auto get_offset() const -> Vector2 { return offset_; }
    [[nodiscard]] auto get_direction() const -> direction { return dir_; }
    [[nodiscard]] auto get_state() -> animation_state & { return state_; }

    void set_image(const std::string &image) { image_ = image; }
    void set_ani(const std::string &ani);
    void set_offset(const Vector2 offset) { offset_ = offset; }
    void set_direction(const direction dir) { dir_ = dir; }

    void update(float dt) override;
    void draw() const override;

  private:
    std::string image_;
    std::string ani_;
    animation *animation_ = nullptr;
    animation_state state_{};
    Vector2 offset_{};
    direction dir_ = direction::down;
};
