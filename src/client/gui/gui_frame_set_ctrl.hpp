#pragma once

#include "gui_control.hpp"

#include <vector>

class gui_frame_set_ctrl : public gui_control {
  public:
    static constexpr float bevel_width = 2.0f;

    void set_column_count(int count);
    void set_row_count(int count);
    void set_column_offset(int index, float offset);
    void set_row_offset(int index, float offset);

    [[nodiscard]] auto get_column_count() const -> int { return static_cast<int>(column_offsets_.size()) + 1; }
    [[nodiscard]] auto get_row_count() const -> int { return static_cast<int>(row_offsets_.size()) + 1; }
    [[nodiscard]] auto get_column_offset(int index) const -> float;
    [[nodiscard]] auto get_row_offset(int index) const -> float;

    void set_border_width(const float width) { border_width_ = width; }
    void set_border_color(const Color color) { border_color_ = color; }
    void set_min_extent(const Vector2 extent) { min_extent_ = extent; }

    std::function<void()> resized;

    void update(float dt) override;
    void draw() const override;

  protected:
    void on_mouse_pressed(int button, Vector2 pt) override;
    void on_mouse_move(Vector2 pt) override;
    void on_mouse_released(int button, Vector2 pt) override;
    void on_mouse_exit() override;

  private:
    [[nodiscard]] auto column_edges() const -> std::vector<float>;
    [[nodiscard]] auto row_edges() const -> std::vector<float>;
    [[nodiscard]] auto border_at(Vector2 pt, bool &vertical) const -> int;
    void layout_frames();

    std::vector<float> column_offsets_;
    std::vector<float> row_offsets_;
    float border_width_ = 4.0f;
    Color border_color_{128, 128, 255, 128};
    Vector2 min_extent_{40, 40};

    int dragging_ = -1;
    bool dragging_vertical_ = false;
    bool resize_cursor_ = false;
};
