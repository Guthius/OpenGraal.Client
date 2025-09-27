#pragma once

#include <limits>
#include <memory>
#include <raylib.h>
#include <vector>

#include "gui_control_profile.hpp"

class gui_control;

struct gui_control_context
{
	std::shared_ptr<gui_control> mouse_focus;
	std::shared_ptr<gui_control> mouse_over_target;
};

using gui_control_ptr = std::shared_ptr<gui_control>;

class gui_control : public std::enable_shared_from_this<gui_control>
{
public:
	virtual ~gui_control() = default;

	virtual void update(float dt);
	virtual void draw() const;

	void add_child(const gui_control_ptr &child);
	void bring_to_front() const;
	void push_to_back() const;
	void clear_controls();
	auto global_to_local_coord(float x, float y) const -> Vector2;
	auto local_to_global_coord(Vector2 local) const -> Vector2;
	void show() { visible_ = true; }
	void hide() { visible_ = false; }

	[[nodiscard]] auto get_bounds() const -> const Rectangle & { return bounds_; }
	[[nodiscard]] auto get_position() const -> Vector2 { return Vector2(bounds_.x, bounds_.y); }
	[[nodiscard]] auto get_size() const -> Vector2 { return Vector2(bounds_.width, bounds_.height); }
	[[nodiscard]] auto get_profile() const -> const std::shared_ptr<gui_control_profile> &;
	[[nodiscard]] auto get_parent() const -> const gui_control_ptr &;
	[[nodiscard]] auto get_children() const -> const std::vector<gui_control_ptr> &;
	[[nodiscard]] auto is_visible() const -> bool { return visible_; }
	[[nodiscard]] auto is_disabled() const -> bool { return disabled_; }
	[[nodiscard]] auto is_mouse_over() const -> bool { return mouse_over_; }

	void set_position(Vector2 position);
	void set_size(Vector2 size);
	void set_profile(const std::shared_ptr<gui_control_profile> &profile) { profile_ = profile; }
	void set_visible(const bool visible) { visible_ = visible; }
	void set_disabled(const bool disabled) { disabled_ = disabled; }

protected:
	[[nodiscard]] auto get_context() -> gui_control_context &;
	[[nodiscard]] auto get_child_at(Vector2 pt) -> gui_control_ptr;
	[[nodiscard]] auto contains(Vector2 pt) const -> bool;

	void draw_children() const;
	void capture_mouse();
	void release_mouse();

	virtual void on_mouse_enter();
	virtual void on_mouse_move(Vector2 pt);
	virtual void on_mouse_exit();
	virtual void on_mouse_pressed(int button, Vector2 pt);
	virtual void on_mouse_released(int button, Vector2 pt);

private:
	void handle_input();

	Rectangle bounds_{};
	gui_control_context context_{};
	std::shared_ptr<gui_control_profile> profile_;
	gui_control_ptr parent_;
	std::vector<gui_control_ptr> children_;
	bool visible_ = true;
	bool disabled_ = false;
	bool mouse_over_ = false;

	int last_mouse_x_ = std::numeric_limits<int>::min();
	int last_mouse_y_ = std::numeric_limits<int>::min();
};
