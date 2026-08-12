#pragma once

#include "gui_control_profile.hpp"

#include <functional>
#include <limits>
#include <memory>
#include <raylib.h>
#include <string>
#include <vector>

class gui_control;

using gui_control_ptr = std::shared_ptr<gui_control>;

enum class gui_horiz_sizing : uint8_t { right,
    width,
    left,
    center,
    relative,
};

enum class gui_vert_sizing : uint8_t { bottom,
    height,
    top,
    center,
    relative,
};

class gui_control : public std::enable_shared_from_this<gui_control> {
  public:
    virtual ~gui_control() = default;

    virtual void update(float dt);
    virtual void draw() const;

    void add_child(const gui_control_ptr &child);
    void remove_child(const gui_control_ptr &child);
    void bring_to_front() const;
    void push_to_back() const;
    void clear_controls();
    auto global_to_local_coord(float x, float y) const -> Vector2;
    auto local_to_global_coord(Vector2 local) const -> Vector2;
    void show() { visible_ = true; }
    void hide() { visible_ = false; }

    [[nodiscard]] auto get_name() const -> const std::string & { return name_; }
    [[nodiscard]] auto get_hint() const -> const std::string & { return hint_; }
    [[nodiscard]] auto get_show_hint() const -> bool { return show_hint_; }
    [[nodiscard]] auto get_bounds() const -> const Rectangle & { return bounds_; }
    [[nodiscard]] auto get_position() const -> Vector2 { return Vector2(bounds_.x, bounds_.y); }
    [[nodiscard]] auto get_size() const -> Vector2 { return Vector2(bounds_.width, bounds_.height); }
    [[nodiscard]] auto get_profile() const -> const std::shared_ptr<gui_control_profile> &;
    [[nodiscard]] auto get_parent() const -> const gui_control_ptr &;
    [[nodiscard]] auto get_children() const -> const std::vector<gui_control_ptr> &;
    [[nodiscard]] auto is_visible() const -> bool { return visible_; }
    [[nodiscard]] auto is_disabled() const -> bool { return disabled_; }
    [[nodiscard]] auto is_mouse_over() const -> bool { return mouse_over_; }
    [[nodiscard]] auto has_focus() const -> bool;

    void set_name(std::string name) { name_ = std::move(name); }
    void set_hint(std::string hint) { hint_ = std::move(hint); }
    void set_show_hint(const bool show) { show_hint_ = show; }
    void set_can_resize(const bool can_resize) { can_resize_ = can_resize; }
    void set_min_size(const Vector2 size) { min_size_ = size; }
    void set_horiz_sizing(const gui_horiz_sizing sizing) { horiz_sizing_ = sizing; }
    void set_vert_sizing(const gui_vert_sizing sizing) { vert_sizing_ = sizing; }
    void set_next_focus(const gui_control_ptr &next) { next_focus_ = next; }

    [[nodiscard]]
    auto get_next_focus() const -> gui_control_ptr { return next_focus_.lock(); }

    void set_position(Vector2 position);

    void set_size(Vector2 size);
    void set_profile(const std::shared_ptr<gui_control_profile> &profile) { profile_ = profile; }
    void set_visible(const bool visible) { visible_ = visible; }
    void set_disabled(const bool disabled) { disabled_ = disabled; }

    void focus();
    void blur();

    std::function<void()> clicked;
    std::function<void()> mouse_pressed;

  protected:
    [[nodiscard]] auto get_child_at(Vector2 pt) -> gui_control_ptr;
    [[nodiscard]] auto contains(Vector2 pt) const -> bool;
    [[nodiscard]] auto is_resizing() const -> bool { return resizing_edges_ != 0; }

    void draw_children() const;
    void capture_mouse();
    void release_mouse();

    virtual void on_mouse_enter();
    virtual void on_mouse_move(Vector2 pt);
    virtual void on_mouse_exit();
    virtual void on_mouse_pressed(int button, Vector2 pt);
    virtual void on_mouse_released(int button, Vector2 pt);
    virtual void on_focus();
    virtual void on_blur();

  private:
    void handle_input();
    void draw_hint() const;
    [[nodiscard]] auto resize_edges_at(Vector2 pt) const -> uint8_t;

    std::string name_;
    std::string hint_;
    bool show_hint_ = true;
    bool can_resize_ = false;
    Vector2 min_size_{8, 2};
    gui_horiz_sizing horiz_sizing_ = gui_horiz_sizing::right;
    gui_vert_sizing vert_sizing_ = gui_vert_sizing::bottom;
    uint8_t resizing_edges_ = 0;
    Rectangle resize_start_{};
    Vector2 resize_grab_{};
    bool sizing_cursor_ = false;
    Rectangle bounds_{};
    std::shared_ptr<gui_control_profile> profile_;
    gui_control_ptr parent_;
    std::vector<gui_control_ptr> children_;
    bool visible_ = true;
    bool disabled_ = false;
    bool mouse_over_ = false;
    std::weak_ptr<gui_control> next_focus_;

    int last_mouse_x_ = std::numeric_limits<int>::min();
    int last_mouse_y_ = std::numeric_limits<int>::min();
};
