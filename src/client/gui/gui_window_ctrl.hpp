#pragma once

#include "gui_text_ctrl.hpp"

#include <utility>
#include <vector>

class gui_window_ctrl final : public gui_text_ctrl {
  public:
    void update(float dt) override;
    void draw() const override;

    [[nodiscard]]
    auto is_draggable() const -> bool { return draggable_; }
    [[nodiscard]] auto get_can_close() const -> bool { return can_close_; }
    [[nodiscard]] auto get_can_maximize() const -> bool { return can_maximize_; }
    [[nodiscard]] auto is_maximized() const -> bool { return maximized_; }

    void set_draggable(const bool draggable) { draggable_ = draggable; }
    void set_can_close(const bool can_close) { can_close_ = can_close; }
    void set_can_maximize(const bool can_maximize) { can_maximize_ = can_maximize; }
    void set_maximized(bool maximized);

    std::function<void()> close_requested;

  protected:
    void on_mouse_pressed(int button, Vector2 pt) override;
    void on_mouse_released(int button, Vector2 pt) override;
    void on_mouse_move(Vector2 pt) override;
    void on_mouse_exit() override;

  private:
    enum class title_button : uint8_t {
        close,
        maximize,
    };

    [[nodiscard]] auto title_buttons() const -> std::vector<std::pair<title_button, Rectangle>>;
    [[nodiscard]] auto button_at(Vector2 pt) const -> int;

    bool draggable_ = true;
    bool dragging_ = false;
    Vector2 drag_offset_{};
    bool can_close_ = false;
    bool can_maximize_ = false;
    bool maximized_ = false;
    Rectangle restore_bounds_{};
    bool animating_ = false;
    float anim_t_ = 0.0f;
    Rectangle anim_from_{};
    Rectangle anim_to_{};
    int hover_button_ = -1;
    double last_title_press_ = 0.0;
};
