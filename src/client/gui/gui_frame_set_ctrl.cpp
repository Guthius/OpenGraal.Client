#include "gui_frame_set_ctrl.hpp"

#include <algorithm>

namespace {
    constexpr Color bevel_outer_dark{66, 66, 66, 255};
    constexpr Color bevel_inner_dark{0, 0, 0, 255};
    constexpr Color bevel_inner_light{41, 82, 156, 255};
    constexpr Color bevel_outer_light{255, 255, 255, 255};

    void draw_edges(const Rectangle r, const Color top_left, const Color bottom_right) {
        DrawRectangleRec({r.x, r.y, r.width, 1}, top_left);
        DrawRectangleRec({r.x, r.y, 1, r.height}, top_left);
        DrawRectangleRec({r.x, r.y + r.height - 1, r.width, 1}, bottom_right);
        DrawRectangleRec({r.x + r.width - 1, r.y, 1, r.height}, bottom_right);
    }

    void draw_bevel(const Rectangle cell) {
        draw_edges(cell, bevel_outer_dark, bevel_outer_light);
        draw_edges({cell.x + 1, cell.y + 1, cell.width - 2, cell.height - 2}, bevel_inner_dark, bevel_inner_light);
    }
}

void gui_frame_set_ctrl::set_column_count(const int count) {
    column_offsets_.assign(static_cast<size_t>(std::max(1, count)) - 1, 0.0f);

    const auto width = get_size().x;
    for (size_t i = 0; i < column_offsets_.size(); ++i) {
        column_offsets_[i] = width * (static_cast<float>(i) + 1.0f) / static_cast<float>(count);
    }
}

void gui_frame_set_ctrl::set_row_count(const int count) {
    row_offsets_.assign(static_cast<size_t>(std::max(1, count)) - 1, 0.0f);

    const auto height = get_size().y;
    for (size_t i = 0; i < row_offsets_.size(); ++i) {
        row_offsets_[i] = height * (static_cast<float>(i) + 1.0f) / static_cast<float>(count);
    }
}

void gui_frame_set_ctrl::set_column_offset(const int index, const float offset) {
    if (index >= 1 && index <= static_cast<int>(column_offsets_.size())) {
        column_offsets_[static_cast<size_t>(index) - 1] = offset;
    }
}

void gui_frame_set_ctrl::set_row_offset(const int index, const float offset) {
    if (index >= 1 && index <= static_cast<int>(row_offsets_.size())) {
        row_offsets_[static_cast<size_t>(index) - 1] = offset;
    }
}

auto gui_frame_set_ctrl::get_column_offset(const int index) const -> float {
    if (index <= 0) {
        return 0.0f;
    }

    return index <= static_cast<int>(column_offsets_.size())
               ? column_offsets_[static_cast<size_t>(index) - 1]
               : get_size().x;
}

auto gui_frame_set_ctrl::get_row_offset(const int index) const -> float {
    if (index <= 0) {
        return 0.0f;
    }

    return index <= static_cast<int>(row_offsets_.size())
               ? row_offsets_[static_cast<size_t>(index) - 1]
               : get_size().y;
}

auto gui_frame_set_ctrl::column_edges() const -> std::vector<float> {
    std::vector<float> edges{0.0f};
    edges.insert(edges.end(), column_offsets_.begin(), column_offsets_.end());
    edges.push_back(get_size().x);

    return edges;
}

auto gui_frame_set_ctrl::row_edges() const -> std::vector<float> {
    std::vector<float> edges{0.0f};
    edges.insert(edges.end(), row_offsets_.begin(), row_offsets_.end());
    edges.push_back(get_size().y);

    return edges;
}

void gui_frame_set_ctrl::layout_frames() {
    const auto columns = column_edges();
    const auto rows = row_edges();
    const auto &children = get_children();

    size_t child = 0;
    for (size_t row = 0; row + 1 < rows.size(); ++row) {
        for (size_t column = 0; column + 1 < columns.size(); ++column) {
            if (child >= children.size()) {
                return;
            }

            const auto x = columns[column] + (column > 0 ? border_width_ : 0.0f) + bevel_width;
            const auto y = rows[row] + (row > 0 ? border_width_ : 0.0f) + bevel_width;

            children[child]->set_position({x, y});
            children[child]->set_size({
                std::max(0.0f, columns[column + 1] - x - bevel_width),
                std::max(0.0f, rows[row + 1] - y - bevel_width),
            });

            ++child;
        }
    }
}

auto gui_frame_set_ctrl::border_at(const Vector2 pt, bool &vertical) const -> int {
    for (size_t i = 0; i < column_offsets_.size(); ++i) {
        if (pt.x >= column_offsets_[i] - bevel_width && pt.x < column_offsets_[i] + border_width_ + bevel_width) {
            vertical = true;

            return static_cast<int>(i);
        }
    }

    for (size_t i = 0; i < row_offsets_.size(); ++i) {
        if (pt.y >= row_offsets_[i] - bevel_width && pt.y < row_offsets_[i] + border_width_ + bevel_width) {
            vertical = false;

            return static_cast<int>(i);
        }
    }

    return -1;
}

void gui_frame_set_ctrl::on_mouse_pressed(const int button, const Vector2 pt) {
    if (button != MOUSE_BUTTON_LEFT) {
        return;
    }

    auto vertical = false;
    if (const auto border = border_at(pt, vertical); border >= 0) {
        dragging_ = border;
        dragging_vertical_ = vertical;

        capture_mouse();
    }
}

void gui_frame_set_ctrl::on_mouse_move(const Vector2 pt) {
    if (dragging_ < 0) {
        auto vertical = false;
        const auto over = border_at(pt, vertical) >= 0;

        if (over) {
            SetMouseCursor(vertical ? MOUSE_CURSOR_RESIZE_EW : MOUSE_CURSOR_RESIZE_NS);
        } else if (resize_cursor_) {
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }

        resize_cursor_ = over;

        return;
    }

    const auto index = static_cast<size_t>(dragging_);

    if (dragging_vertical_) {
        const auto edges = column_edges();
        const auto low = edges[index] + (index > 0 ? border_width_ : 0.0f) + min_extent_.x;
        const auto high = edges[index + 2] - border_width_ - min_extent_.x;

        column_offsets_[index] = std::clamp(pt.x - (border_width_ / 2.0f), low, std::max(low, high));
    } else {
        const auto edges = row_edges();
        const auto low = edges[index] + (index > 0 ? border_width_ : 0.0f) + min_extent_.y;
        const auto high = edges[index + 2] - border_width_ - min_extent_.y;

        row_offsets_[index] = std::clamp(pt.y - (border_width_ / 2.0f), low, std::max(low, high));
    }

    layout_frames();

    if (resized) {
        resized();
    }
}

void gui_frame_set_ctrl::on_mouse_released(const int /*button*/, const Vector2 /*pt*/) {
    if (dragging_ >= 0) {
        dragging_ = -1;

        release_mouse();
    }
}

void gui_frame_set_ctrl::on_mouse_exit() {
    gui_control::on_mouse_exit();

    if (resize_cursor_) {
        resize_cursor_ = false;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }
}

void gui_frame_set_ctrl::update(const float dt) {
    layout_frames();

    gui_control::update(dt);
}

void gui_frame_set_ctrl::draw() const {
    const auto bounds = get_bounds();

    for (const auto offset : column_offsets_) {
        DrawRectangleRec({bounds.x + offset, bounds.y, border_width_, bounds.height}, border_color_);
    }

    for (const auto offset : row_offsets_) {
        DrawRectangleRec({bounds.x, bounds.y + offset, bounds.width, border_width_}, border_color_);
    }

    const auto columns = column_edges();
    const auto rows = row_edges();

    for (size_t row = 0; row + 1 < rows.size(); ++row) {
        for (size_t column = 0; column + 1 < columns.size(); ++column) {
            const auto x = bounds.x + columns[column] + (column > 0 ? border_width_ : 0.0f);
            const auto y = bounds.y + rows[row] + (row > 0 ? border_width_ : 0.0f);

            draw_bevel({x, y, bounds.x + columns[column + 1] - x, bounds.y + rows[row + 1] - y});
        }
    }

    draw_children();
}
