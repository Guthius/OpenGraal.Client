#pragma once

#include "gui_button_base_ctrl.hpp"
#include "gui_drawing_panel.hpp"

class gui_button_ctrl final : public gui_button_base_ctrl {
  public:
    [[nodiscard]] auto get_icon() -> const std::shared_ptr<gui_drawing_panel> &;
    void set_icon_size(float width, float height);

    void update(float dt) override;
    void draw() const override;

  private:
    std::shared_ptr<gui_drawing_panel> icon_;
    Vector2 icon_size_{20, 20};
};
