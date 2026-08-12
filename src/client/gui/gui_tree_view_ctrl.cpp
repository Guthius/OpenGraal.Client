#include "gui_tree_view_ctrl.hpp"

#include "../font_manager.hpp"

#include <algorithm>

namespace {
    constexpr float row_padding = 2.0f;
    constexpr float text_padding = 4.0f;
    constexpr float indent_width = 12.0f;
    constexpr float expander_size = 8.0f;

    auto split_path(const std::string &path, const std::string &separator) -> std::vector<std::string> {
        const auto delimiter = separator.empty() ? '/' : separator.front();

        std::vector<std::string> parts;
        size_t start = 0;

        while (start <= path.size()) {
            const auto at = path.find(delimiter, start);
            auto part = path.substr(start, at == std::string::npos ? std::string::npos : at - start);

            if (!part.empty()) {
                parts.push_back(std::move(part));
            }

            if (at == std::string::npos) {
                break;
            }

            start = at + 1;
        }

        return parts;
    }

    auto child_named(const gui_tree_view_ctrl::node_ptr &parent, const std::string &name) -> gui_tree_view_ctrl::node_ptr {
        const auto match = std::ranges::find_if(parent->children,
            [&name](const gui_tree_view_ctrl::node_ptr &child) { return child->name == name; });

        return match == parent->children.end() ? nullptr : *match;
    }

    void sort_recursive(const gui_tree_view_ctrl::node_ptr &parent) {
        std::ranges::sort(parent->children,
            [](const gui_tree_view_ctrl::node_ptr &a, const gui_tree_view_ctrl::node_ptr &b) {
                return a->sort_group != b->sort_group ? a->sort_group < b->sort_group : a->name < b->name;
            });

        for (const auto &child : parent->children) {
            sort_recursive(child);
        }
    }

    void flatten(const gui_tree_view_ctrl::node_ptr &parent, const int depth, std::vector<std::pair<gui_tree_view_ctrl::node_ptr, int>> &out) {
        for (const auto &child : parent->children) {
            out.emplace_back(child, depth);

            if (child->expanded) {
                flatten(child, depth + 1, out);
            }
        }
    }
}

auto gui_tree_view_ctrl::node::path(const char separator) const -> std::string {
    const auto owner = parent.lock();
    if (!owner || owner->name.empty()) {
        return name;
    }

    return owner->path(separator) + separator + name;
}

auto gui_tree_view_ctrl::node::depth() const -> int {
    const auto owner = parent.lock();

    return owner ? owner->depth() + 1 : 0;
}

void gui_tree_view_ctrl::fit_height() {
    set_size({get_size().x, static_cast<float>(rows().size()) * row_height()});
}

void gui_tree_view_ctrl::clear_nodes() {
    root_->children.clear();
    selected_.reset();

    fit_height();
}

auto gui_tree_view_ctrl::add_node(const std::string &name) -> node_ptr {
    if (auto existing = child_named(root_, name)) {
        return existing;
    }

    auto child = std::make_shared<node>(node{.name = name, .parent = root_});

    root_->children.push_back(child);
    fit_height();

    return child;
}

auto gui_tree_view_ctrl::add_node_by_path(const std::string &path, const std::string &separator) -> node_ptr {
    auto current = root_;

    for (const auto &part : split_path(path, separator)) {
        auto child = child_named(current, part);
        if (!child) {
            child = std::make_shared<node>(node{.name = part, .parent = current});

            current->children.push_back(child);
        }

        current = child;
    }

    fit_height();

    return current == root_ ? nullptr : current;
}

auto gui_tree_view_ctrl::get_node_by_path(const std::string &path, const std::string &separator) const -> node_ptr {
    auto current = root_;

    for (const auto &part : split_path(path, separator)) {
        current = child_named(current, part);
        if (!current) {
            return nullptr;
        }
    }

    return current == root_ ? nullptr : current;
}

void gui_tree_view_ctrl::sort() {
    sort_recursive(root_);
    fit_height();
}

auto gui_tree_view_ctrl::rows() const -> std::vector<visible_row> {
    std::vector<std::pair<node_ptr, int>> flat;
    flatten(root_, 0, flat);

    std::vector<visible_row> visible;
    visible.reserve(flat.size());

    for (auto &[node, depth] : flat) {
        visible.push_back({.node = std::move(node), .depth = depth});
    }

    return visible;
}

auto gui_tree_view_ctrl::text_profile() const -> const std::shared_ptr<gui_control_profile> & {
    return text_profile_ ? text_profile_ : get_profile();
}

auto gui_tree_view_ctrl::row_height() const -> float {
    const auto profile = text_profile();

    return (profile ? profile->get_font_size() : 12.0f) + row_padding;
}

auto gui_tree_view_ctrl::get_node_at(const Vector2 pt) const -> node_ptr {
    const auto height = row_height();
    if (!contains(pt) || height <= 0.0f) {
        return nullptr;
    }

    const auto visible = rows();
    const auto index = static_cast<size_t>(pt.y / height);

    return index < visible.size() ? visible[index].node : nullptr;
}

void gui_tree_view_ctrl::on_mouse_pressed(int button, const Vector2 pt) {
    const auto height = row_height();
    if (!contains(pt) || height <= 0.0f) {
        return;
    }

    const auto visible = rows();
    const auto index = static_cast<size_t>(pt.y / height);

    if (index >= visible.size()) {
        return;
    }

    const auto &[node, depth] = visible[index];

    if (const auto expander = text_padding + (static_cast<float>(depth) * indent_width);
        !node->children.empty() && pt.x >= expander && pt.x < expander + expander_size) {
        node->expanded = !node->expanded;
        fit_height();

        return;
    }

    selected_ = node;

    if (selected) {
        selected(node, node->path('/'), node->path('.'));
    }
}

void gui_tree_view_ctrl::draw() const {
    const auto profile = get_profile();
    if (profile) {
        profile->draw_frame(get_bounds(), is_mouse_over(), is_disabled());
    }

    const auto text = text_profile();
    if (!text) {
        draw_children();

        return;
    }

    const auto bounds = get_bounds();
    const auto size = text->get_font_size();
    const auto font = load_font(text->get_font(), size);
    const auto height = row_height();
    const auto chosen = selected_.lock();

    const auto visible = rows();
    for (size_t i = 0; i < visible.size(); ++i) {
        const auto y = bounds.y + (static_cast<float>(i) * height);
        if (y + height > bounds.y + bounds.height) {
            break;
        }

        const auto &[node, depth] = visible[i];
        const auto highlighted = node == chosen;

        if (highlighted) {
            DrawRectangleRec({bounds.x, y, bounds.width, height}, text->fill_color_hl);
        }

        const auto indent = text_padding + (static_cast<float>(depth) * indent_width);
        const auto color = highlighted ? text->font_color_hl : text->font_color;

        if (!node->children.empty()) {
            const auto x = bounds.x + indent;
            const auto mid = y + (height / 2);

            if (node->expanded) {
                DrawTriangle({x, mid - 2}, {x + (expander_size / 2), mid + 3}, {x + expander_size, mid - 2}, color);
            } else {
                DrawTriangle({x + 1, mid - 4}, {x + 1, mid + 4}, {x + expander_size - 1, mid}, color);
            }
        }

        DrawTextEx(font, node->name.c_str(), {bounds.x + indent + expander_size + 2, y}, size, 1, color);
    }

    draw_children();
}
