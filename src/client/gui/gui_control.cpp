#include "gui_control.hpp"

#include "../dev_input.hpp"
#include "../font_manager.hpp"

#include <algorithm>
#include <cmath>
#include <ranges>
#include <rlgl.h>

namespace {
    constexpr float hint_delay = 0.5f;
    constexpr float resize_margin = 6.0f;

    constexpr uint8_t edge_left = 1;
    constexpr uint8_t edge_right = 2;
    constexpr uint8_t edge_top = 4;
    constexpr uint8_t edge_bottom = 8;

    namespace context {
        std::shared_ptr<gui_control> mouse_focus = nullptr;
        std::shared_ptr<gui_control> mouse_over_target = nullptr;
        std::shared_ptr<gui_control> focus_target = nullptr;
        std::shared_ptr<gui_control> hint_target = nullptr;
        float hint_elapsed = 0.0f;
    }
}

void gui_control::update(const float dt) {
    if (parent_ == nullptr) {
        handle_input();

        if (context::mouse_over_target != context::hint_target) {
            context::hint_target = context::mouse_over_target;
            context::hint_elapsed = 0.0f;
        } else {
            context::hint_elapsed += dt;
        }
    }

    for (const auto &child : children_) {
        if (child) {
            child->update(dt);
        }
    }
}

void gui_control::draw() const {
    if (!visible_) {
        return;
    }

    if (profile_) {
        profile_->draw_frame(bounds_, mouse_over_, disabled_);
    }

    draw_children();

    if (parent_ == nullptr) {
        draw_hint();
    }
}

void gui_control::draw_hint() const {
    if (context::hint_elapsed < hint_delay) {
        return;
    }

    std::string hint;
    std::shared_ptr<gui_control_profile> profile;

    for (auto c = context::hint_target; c; c = c->parent_) {
        if (c->hint_.empty()) {
            continue;
        }

        if (!c->show_hint_ || c->disabled_) {
            return;
        }

        hint = c->hint_;
        profile = c->profile_;

        break;
    }

    if (hint.empty()) {
        return;
    }

    const auto size = profile ? profile->get_font_size() : 12.0f;
    auto font = profile ? load_font(profile->get_font(), size) : GetFontDefault();
    if (!IsFontValid(font)) {
        font = GetFontDefault();
    }

    const auto text_size = MeasureTextEx(font, hint.c_str(), size, 1.0f);
    const auto width = text_size.x + 8.0f;
    const auto height = text_size.y + 4.0f;
    const auto mouse = dev_input::mouse_position();

    auto x = std::clamp(mouse.x, 0.0f, std::max(0.0f, static_cast<float>(GetScreenWidth()) - width));
    auto y = mouse.y + 22.0f;
    if (y + height > static_cast<float>(GetScreenHeight())) {
        y = mouse.y - height - 2.0f;
    }

    DrawRectangleRec({x, y, width, height}, {255, 255, 225, 255});
    DrawRectangleLinesEx({x, y, width, height}, 1, BLACK);
    DrawTextEx(font, hint.c_str(), {x + 4, y + 2}, size, 1.0f, BLACK);
}

void gui_control::add_child(const gui_control_ptr &child) {
    if (!child) {
        return;
    }

    if (const auto previous = child->parent_) {
        previous->remove_child(child);
    }

    children_.push_back(child);

    child->parent_ = shared_from_this();
}

void gui_control::remove_child(const gui_control_ptr &child) {
    if (!child) {
        return;
    }

    std::erase(children_, child);

    child->parent_.reset();
}

void gui_control::bring_to_front() const {
    if (!parent_) {
        return;
    }

    auto &siblings = parent_->children_;
    for (size_t i = 0; i < siblings.size(); ++i) {
        if (siblings[i].get() == this) {
            const auto self = siblings[i];

            siblings.erase(siblings.begin() + static_cast<long long>(i));
            siblings.push_back(self);

            break;
        }
    }
}

void gui_control::push_to_back() const {
    if (!parent_) {
        return;
    }

    auto &siblings = parent_->children_;
    for (size_t i = 0; i < siblings.size(); ++i) {
        if (siblings[i].get() == this) {
            const auto self = siblings[i];

            siblings.erase(siblings.begin() + static_cast<long long>(i));
            siblings.insert(siblings.begin(), self);

            break;
        }
    }
}

void gui_control::clear_controls() {
    for (const auto &child : children_) {
        if (child) {
            child->parent_.reset();
        }
    }

    children_.clear();
}

auto gui_control::global_to_local_coord(const float x, const float y) const -> Vector2 {
    Vector2 pt{(bounds_.x), (bounds_.y)};

    auto cur = parent_;
    while (cur) {
        pt.x += cur->bounds_.x;
        pt.y += cur->bounds_.y;

        cur = cur->parent_;
    }

    return {x - pt.x, y - pt.y};
}

auto gui_control::local_to_global_coord(const Vector2 local) const -> Vector2 {
    Vector2 pt{(bounds_.x), (bounds_.y)};

    auto cur = parent_;
    while (cur) {
        pt.x += cur->bounds_.x;
        pt.y += cur->bounds_.y;

        cur = cur->parent_;
    }

    return Vector2{local.x + pt.x, local.y + pt.y};
}

auto gui_control::get_profile() const -> const std::shared_ptr<gui_control_profile> & {
    return profile_;
}

auto gui_control::get_parent() const -> const gui_control_ptr & {
    return parent_;
}

auto gui_control::get_children() const -> const std::vector<gui_control_ptr> & {
    return children_;
}

auto gui_control::has_focus() const -> bool {
    return context::focus_target == shared_from_this();
}

void gui_control::set_position(const Vector2 position) {
    bounds_.x = position.x;
    bounds_.y = position.y;
}

void gui_control::set_size(const Vector2 size) {
    const Vector2 old{bounds_.width, bounds_.height};
    if (size.x == old.x && size.y == old.y) {
        return;
    }

    bounds_.width = size.x;
    bounds_.height = size.y;

    if (old.x == 0 && old.y == 0) {
        return;
    }

    for (const auto &child : children_) {
        if (!child) {
            continue;
        }

        auto rect = child->bounds_;

        switch (child->horiz_sizing_) {
        case gui_horiz_sizing::right:
            break;

        case gui_horiz_sizing::width:
            rect.width = std::max(0.0f, rect.width + size.x - old.x);
            break;

        case gui_horiz_sizing::left:
            rect.x += size.x - old.x;
            break;

        case gui_horiz_sizing::center:
            rect.x = std::round((size.x - rect.width) / 2.0f);
            break;

        case gui_horiz_sizing::relative:
            if (old.x > 0) {
                rect.x = rect.x * size.x / old.x;
                rect.width = rect.width * size.x / old.x;
            }
            break;
        }

        switch (child->vert_sizing_) {
        case gui_vert_sizing::bottom:
            break;

        case gui_vert_sizing::height:
            rect.height = std::max(0.0f, rect.height + size.y - old.y);
            break;

        case gui_vert_sizing::top:
            rect.y += size.y - old.y;
            break;

        case gui_vert_sizing::center:
            rect.y = std::round((size.y - rect.height) / 2.0f);
            break;

        case gui_vert_sizing::relative:
            if (old.y > 0) {
                rect.y = rect.y * size.y / old.y;
                rect.height = rect.height * size.y / old.y;
            }
            break;
        }

        child->set_position({rect.x, rect.y});
        child->set_size({rect.width, rect.height});
    }
}

void gui_control::focus() {
    auto focus_target = context::focus_target;
    if (focus_target == shared_from_this()) {
        return;
    }

    context::focus_target = shared_from_this();
    on_focus();

    if (focus_target != nullptr) {
        focus_target->on_blur();
    }
}

void gui_control::blur() {
    if (context::focus_target != shared_from_this()) {
        return;
    }

    context::focus_target.reset();
    on_blur();
}

auto gui_control::get_child_at(const Vector2 pt) -> gui_control_ptr {
    if (!visible_) {
        return nullptr;
    }

    const Vector2 local_pt{pt.x - bounds_.x, pt.y - bounds_.y};

    for (const auto &child : std::ranges::reverse_view(children_)) {
        if (child && child->visible_ && !child->disabled_ && CheckCollisionPointRec(local_pt, child->bounds_)) {
            if (auto c = child->get_child_at(local_pt)) {
                return c;
            }
        }
    }

    return parent_ == nullptr ? nullptr : shared_from_this();
}

auto gui_control::contains(const Vector2 pt) const -> bool {
    return pt.x >= 0 && pt.y >= 0 && pt.x < bounds_.width && pt.y < bounds_.height;
}

void gui_control::draw_children() const {
    rlPushMatrix();
    rlTranslatef(bounds_.x, bounds_.y, 0.0f);

    for (const auto &child : children_) {
        if (child && child->is_visible()) {
            child->draw();
        }
    }

    rlPopMatrix();
}

void gui_control::capture_mouse() {
    context::mouse_focus = shared_from_this();
}

void gui_control::release_mouse() {
    context::mouse_focus.reset();
}

auto gui_control::resize_edges_at(const Vector2 pt) const -> uint8_t {
    if (!can_resize_ || !contains(pt)) {
        return 0;
    }

    uint8_t edges = 0;

    if (pt.x < resize_margin) {
        edges |= edge_left;
    }
    if (pt.x >= bounds_.width - resize_margin) {
        edges |= edge_right;
    }
    if (pt.y < resize_margin) {
        edges |= edge_top;
    }
    if (pt.y >= bounds_.height - resize_margin) {
        edges |= edge_bottom;
    }

    return edges;
}

void gui_control::on_mouse_enter() {
    mouse_over_ = true;
}

void gui_control::on_mouse_move(const Vector2 pt) {
    if (resizing_edges_ != 0) {
        const auto mouse = local_to_global_coord(pt);
        const auto dx = mouse.x - resize_grab_.x;
        const auto dy = mouse.y - resize_grab_.y;

        auto rect = resize_start_;

        if ((resizing_edges_ & edge_left) != 0) {
            const auto give = std::min(dx, resize_start_.width - min_size_.x);
            rect.x += give;
            rect.width -= give;
        }
        if ((resizing_edges_ & edge_right) != 0) {
            rect.width = std::max(min_size_.x, resize_start_.width + dx);
        }
        if ((resizing_edges_ & edge_top) != 0) {
            const auto give = std::min(dy, resize_start_.height - min_size_.y);
            rect.y += give;
            rect.height -= give;
        }
        if ((resizing_edges_ & edge_bottom) != 0) {
            rect.height = std::max(min_size_.y, resize_start_.height + dy);
        }

        set_position({std::round(rect.x), std::round(rect.y)});
        set_size({std::round(rect.width), std::round(rect.height)});

        return;
    }

    if (!can_resize_ || is_disabled()) {
        return;
    }

    if (const auto edges = resize_edges_at(pt); edges != 0) {
        const auto cursor =
            edges == (edge_left | edge_top) || edges == (edge_right | edge_bottom) ? MOUSE_CURSOR_RESIZE_NWSE
            : edges == (edge_right | edge_top) || edges == (edge_left | edge_bottom)
                ? MOUSE_CURSOR_RESIZE_NESW
            : (edges & (edge_left | edge_right)) != 0
                ? MOUSE_CURSOR_RESIZE_EW
                : MOUSE_CURSOR_RESIZE_NS;

        SetMouseCursor(cursor);
        sizing_cursor_ = true;
    } else if (sizing_cursor_) {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        sizing_cursor_ = false;
    }
}

void gui_control::on_mouse_exit() {
    mouse_over_ = false;

    if (sizing_cursor_) {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        sizing_cursor_ = false;
    }
}

void gui_control::on_mouse_pressed(const int button, const Vector2 pt) {
    if (button != MOUSE_BUTTON_LEFT || is_disabled()) {
        return;
    }

    if (const auto edges = resize_edges_at(pt); edges != 0) {
        resizing_edges_ = edges;
        resize_start_ = bounds_;
        resize_grab_ = local_to_global_coord(pt);

        capture_mouse();
    }
}

void gui_control::on_mouse_released(const int button, Vector2 /*pt*/) {
    if (button == MOUSE_BUTTON_LEFT && resizing_edges_ != 0) {
        resizing_edges_ = 0;

        release_mouse();
    }
}

void gui_control::on_focus() {
}

void gui_control::on_blur() {
}

void gui_control::handle_input() {
    if (dev_input::key_pressed(KEY_TAB)) {
        if (const auto focused = context::focus_target) {
            if (const auto next = focused->get_next_focus()) {
                next->focus();
            }
        }
    }

    auto handle_mouse_over_change = [&](const Vector2 pt) -> void {
        auto get_chain = [](gui_control_ptr c) {
            std::vector<gui_control_ptr> v;
            for (; c; c = c->parent_)
                v.push_back(c);
            return v;
        };

        const auto new_mouse_over_target = get_child_at(pt);
        if (context::mouse_over_target == new_mouse_over_target) {
            return;
        }

        auto old_chain = get_chain(context::mouse_over_target);
        auto new_chain = get_chain(new_mouse_over_target);

        while (!old_chain.empty() && !new_chain.empty() && old_chain.back() == new_chain.back()) {
            old_chain.pop_back();
            new_chain.pop_back();
        }

        for (const auto &control : old_chain)
            control->on_mouse_exit();
        for (const auto &control : std::ranges::reverse_view(new_chain)) {
            control->on_mouse_enter();
        }

        context::mouse_over_target = new_mouse_over_target;
    };

    auto handle_mouse_moved = [&](const Vector2 pt) -> void {
        if (const auto mouse_focus = context::mouse_focus) {
            const auto local_pt = mouse_focus->global_to_local_coord(pt.x, pt.y);

            mouse_focus->on_mouse_move(local_pt);

            return;
        }

        handle_mouse_over_change(pt);

        if (CheckCollisionPointRec(pt, bounds_)) {
            const auto target = get_child_at(pt);
            if (!target) {
                return;
            }

            auto local_pt = target->global_to_local_coord(pt.x, pt.y);
            for (auto c = target; c; c = c->parent_) {
                c->on_mouse_move(local_pt);

                local_pt.x += c->bounds_.x;
                local_pt.y += c->bounds_.y;
            }
        }
    };

    auto handle_mouse_pressed = [&](const int button, const Vector2 pt) -> bool {
        if (const auto mouse_focus = context::mouse_focus) {
            const auto local_pt = mouse_focus->global_to_local_coord(pt.x, pt.y);

            mouse_focus->on_mouse_pressed(button, local_pt);

            if (mouse_focus->mouse_pressed) {
                mouse_focus->mouse_pressed();
            }

            return true;
        }

        const auto target = get_child_at(pt);
        if (!target) {
            return false;
        }

        if (target->mouse_pressed) {
            target->mouse_pressed();
        }

        auto local_pt = target->global_to_local_coord(pt.x, pt.y);
        for (auto c = target; c; c = c->parent_) {
            c->on_mouse_pressed(button, local_pt);

            local_pt.x += c->bounds_.x;
            local_pt.y += c->bounds_.y;
        }

        return true;
    };

    auto handle_mouse_released = [&](const int button, const Vector2 pt) -> void {
        if (const auto mouse_focus = context::mouse_focus) {
            const auto local_pt = mouse_focus->global_to_local_coord(pt.x, pt.y);

            mouse_focus->on_mouse_released(button, local_pt);

            return;
        }

        const auto target = get_child_at(pt);
        if (!target) {
            return;
        }

        auto local_pt = target->global_to_local_coord(pt.x, pt.y);
        for (auto c = target; c; c = c->parent_) {
            c->on_mouse_released(button, local_pt);

            local_pt.x += c->bounds_.x;
            local_pt.y += c->bounds_.y;
        }
    };

    if (disabled_) {
        if (context::mouse_over_target) {
            for (auto control = context::mouse_over_target; control; control = control->parent_) {
                control->on_mouse_exit();
            }

            context::mouse_over_target = nullptr;
        }

        return;
    }

    const auto [mouse_x, mouse_y] = dev_input::mouse_position();

    if (static_cast<int>(mouse_x) != last_mouse_x_ || static_cast<int>(mouse_y) != last_mouse_y_) {
        handle_mouse_moved(Vector2(
            static_cast<float>(mouse_x),
            static_cast<float>(mouse_y)));

        last_mouse_x_ = static_cast<int>(mouse_x);
        last_mouse_y_ = static_cast<int>(mouse_y);
    }

    auto check_mouse_button = [&](const int button) -> void {
        if (dev_input::mouse_pressed(button)) {
            context::hint_elapsed = 0.0f;

            auto handled = handle_mouse_pressed(button, Vector2(mouse_x, mouse_y));

            if (!handled && context::focus_target) {
                context::focus_target->blur();
            }
        }

        if (dev_input::mouse_released(button)) {
            handle_mouse_released(button, Vector2(mouse_x, mouse_y));
        }
    };

    check_mouse_button(MOUSE_BUTTON_LEFT);
    check_mouse_button(MOUSE_BUTTON_RIGHT);
    check_mouse_button(MOUSE_BUTTON_MIDDLE);
}

bool gui_has_focus() {
    return context::focus_target != nullptr;
}
