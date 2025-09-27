#include "gui_control.hpp"

#include <functional>
#include <ranges>
#include <rlgl.h>

void gui_control::update(const float dt)
{
	if (parent_ == nullptr)
	{
		handle_input();
	}

	for (const auto &child: children_)
	{
		if (child)
		{
			child->update(dt);
		}
	}
}

void gui_control::draw() const
{
	if (!visible_)
	{
		return;
	}

	if (profile_)
	{
		profile_->draw_frame(bounds_, mouse_over_, disabled_);
	}

	draw_children();
}

void gui_control::add_child(const gui_control_ptr &child)
{
	if (!child)
	{
		return;
	}

	children_.push_back(child);

	child->parent_ = shared_from_this();
}

void gui_control::bring_to_front() const
{
	if (!parent_)
	{
		return;
	}

	auto &siblings = parent_->children_;
	for (size_t i = 0; i < siblings.size(); ++i)
	{
		if (siblings[i].get() == this)
		{
			const auto self = siblings[i];

			siblings.erase(siblings.begin() + static_cast<long long>(i));
			siblings.push_back(self);

			break;
		}
	}
}

void gui_control::push_to_back() const
{
	if (!parent_)
	{
		return;
	}

	auto &siblings = parent_->children_;
	for (size_t i = 0; i < siblings.size(); ++i)
	{
		if (siblings[i].get() == this)
		{
			const auto self = siblings[i];

			siblings.erase(siblings.begin() + static_cast<long long>(i));
			siblings.insert(siblings.begin(), self);

			break;
		}
	}
}

void gui_control::clear_controls()
{
	children_.clear();
}

auto gui_control::global_to_local_coord(const float x, const float y) const -> Vector2
{
	Vector2 pt{(bounds_.x), (bounds_.y)};

	auto cur = parent_;
	while (cur)
	{
		pt.x += cur->bounds_.x;
		pt.y += cur->bounds_.y;

		cur = cur->parent_;
	}

	return {x - pt.x, y - pt.y};
}

auto gui_control::local_to_global_coord(const Vector2 local) const -> Vector2
{
	Vector2 pt{(bounds_.x), (bounds_.y)};

	auto cur = parent_;
	while (cur)
	{
		pt.x += cur->bounds_.x;
		pt.y += cur->bounds_.y;

		cur = cur->parent_;
	}

	return Vector2{local.x + pt.x, local.y + pt.y};
}

auto gui_control::get_profile() const -> const std::shared_ptr<gui_control_profile> &
{
	return profile_;
}

auto gui_control::get_parent() const -> const gui_control_ptr &
{
	return parent_;
}

auto gui_control::get_children() const -> const std::vector<gui_control_ptr> &
{
	return children_;
}

void gui_control::set_position(const Vector2 position)
{
	bounds_.x = position.x;
	bounds_.y = position.y;
}

void gui_control::set_size(const Vector2 size)
{
	bounds_.width = size.x;
	bounds_.height = size.y;
}

auto gui_control::get_context() -> gui_control_context &
{
	auto root = this;
	for (; root->parent_; root = root->parent_.get())
	{
	}

	return root->context_;
}

auto gui_control::get_child_at(const Vector2 pt) -> gui_control_ptr
{
	if (!visible_)
	{
		return nullptr;
	}

	const Vector2 local_pt{pt.x - bounds_.x, pt.y - bounds_.y};

	for (const auto &child: std::ranges::reverse_view(children_))
	{
		if (child && child->visible_ && !child->disabled_ && CheckCollisionPointRec(local_pt, child->bounds_))
		{
			if (auto c = child->get_child_at(local_pt))
			{
				return c;
			}
		}
	}

	return shared_from_this();
}

auto gui_control::contains(const Vector2 pt) const -> bool
{
	return pt.x >= 0 && pt.y >= 0 && pt.x < bounds_.width && pt.y < bounds_.height;
}

void gui_control::draw_children() const
{
	rlPushMatrix();
	rlTranslatef(bounds_.x, bounds_.y, 0.0f);

	for (const auto &child: children_)
	{
		if (child)
		{
			child->draw();
		}
	}

	rlPopMatrix();
}

void gui_control::capture_mouse()
{
	get_context().mouse_focus = shared_from_this();
}

void gui_control::release_mouse()
{
	get_context().mouse_focus.reset();
}

void gui_control::on_mouse_enter()
{
	mouse_over_ = true;
}

void gui_control::on_mouse_move(const Vector2 pt)
{
}

void gui_control::on_mouse_exit()
{
	mouse_over_ = false;
}

void gui_control::on_mouse_pressed(int button, Vector2 pt)
{
}

void gui_control::on_mouse_released(int button, Vector2 pt)
{
}

void gui_control::handle_input()
{
	auto &ctx = get_context();

	auto handle_mouse_over_change = [&](const Vector2 pt) -> void {
		auto get_chain = [](gui_control_ptr c) {
			std::vector<gui_control_ptr> v;
			for (; c; c = c->parent_) v.push_back(c);
			return v;
		};

		const auto new_mouse_over_target = get_child_at(pt);
		if (ctx.mouse_over_target == new_mouse_over_target)
		{
			return;
		}

		auto old_chain = get_chain(ctx.mouse_over_target);
		auto new_chain = get_chain(new_mouse_over_target);

		while (!old_chain.empty() && !new_chain.empty() && old_chain.back() == new_chain.back())
		{
			old_chain.pop_back();
			new_chain.pop_back();
		}

		for (const auto &control: old_chain) control->on_mouse_exit();
		for (const auto &control: std::ranges::reverse_view(new_chain))
		{
			control->on_mouse_enter();
		}

		ctx.mouse_over_target = new_mouse_over_target;
	};

	auto handle_mouse_moved = [&](const Vector2 pt) -> void {
		if (const auto mouse_focus = ctx.mouse_focus)
		{
			const auto local_pt = mouse_focus->global_to_local_coord(pt.x, pt.y);

			mouse_focus->on_mouse_move(local_pt);

			return;
		}

		handle_mouse_over_change(pt);

		if (CheckCollisionPointRec(pt, bounds_))
		{
			const auto target = get_child_at(pt);
			if (!target)
			{
				return;
			}

			auto local_pt = target->global_to_local_coord(pt.x, pt.y);
			for (auto c = target; c; c = c->parent_)
			{
				c->on_mouse_move(local_pt);

				local_pt.x += c->bounds_.x;
				local_pt.y += c->bounds_.y;
			}
		}
	};

	auto handle_mouse_pressed = [&](const int button, const Vector2 pt) -> void {
		if (const auto mouse_focus = ctx.mouse_focus)
		{
			const auto local_pt = mouse_focus->global_to_local_coord(pt.x, pt.y);

			mouse_focus->on_mouse_pressed(button, local_pt);

			return;
		}

		const auto target = get_child_at(pt);
		if (!target)
		{
			return;
		}

		auto local_pt = target->global_to_local_coord(pt.x, pt.y);
		for (auto c = target; c; c = c->parent_)
		{
			c->on_mouse_pressed(button, local_pt);

			local_pt.x += c->bounds_.x;
			local_pt.y += c->bounds_.y;
		}
	};

	auto handle_mouse_released = [&](const int button, const Vector2 pt) -> void {
		if (const auto mouse_focus = ctx.mouse_focus)
		{
			const auto local_pt = mouse_focus->global_to_local_coord(pt.x, pt.y);

			mouse_focus->on_mouse_released(button, local_pt);

			return;
		}

		const auto target = get_child_at(pt);
		if (!target)
		{
			return;
		}

		auto local_pt = target->global_to_local_coord(pt.x, pt.y);
		for (auto c = target; c; c = c->parent_)
		{
			c->on_mouse_released(button, local_pt);

			local_pt.x += c->bounds_.x;
			local_pt.y += c->bounds_.y;
		}
	};

	if (disabled_)
	{
		if (ctx.mouse_over_target)
		{
			for (auto control = ctx.mouse_over_target; control; control = control->parent_)
			{
				control->on_mouse_exit();
			}

			ctx.mouse_over_target = nullptr;
		}

		return;
	}

	const auto mouse_x = GetMouseX();
	const auto mouse_y = GetMouseY();

	auto check_mouse_button = [&](const int button) -> void {
		if (IsMouseButtonPressed(button))
		{
			handle_mouse_pressed(button, Vector2(
				static_cast<float>(mouse_x),
				static_cast<float>(mouse_y)));
		}

		if (IsMouseButtonReleased(button))
		{
			handle_mouse_released(button, Vector2(
				static_cast<float>(mouse_x),
				static_cast<float>(mouse_y)));
		}
	};

	check_mouse_button(MOUSE_BUTTON_LEFT);
	check_mouse_button(MOUSE_BUTTON_RIGHT);
	check_mouse_button(MOUSE_BUTTON_MIDDLE);

	if (mouse_x != last_mouse_x_ || mouse_y != last_mouse_y_)
	{
		handle_mouse_moved(Vector2(
			static_cast<float>(mouse_x),
			static_cast<float>(mouse_y)));

		last_mouse_x_ = mouse_x;
		last_mouse_y_ = mouse_y;
	}
}
