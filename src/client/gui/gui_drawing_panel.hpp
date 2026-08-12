#pragma once

#include "gui_control.hpp"

#include <string>
#include <vector>

class gui_drawing_panel : public gui_control {
  public:
    void clear_all() { commands_.clear(); }
    void draw_image(std::string image, Vector2 position);
    void draw_image_stretched(std::string image, Rectangle dest, Rectangle source);
    void draw_image_rectangle(std::string image, Vector2 position, Rectangle source);

    void draw() const override;

  private:
    struct command {
        std::string image;
        Rectangle dest;
        Rectangle source;
    };

    std::vector<command> commands_;
};
