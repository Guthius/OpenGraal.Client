#pragma once

#include "gui_control.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class gui_tree_view_ctrl : public gui_control {
  public:
    struct node {
        std::string name;
        std::vector<std::shared_ptr<node>> children;
        std::weak_ptr<node> parent;
        bool expanded = true;
        int sort_group = 0;

        [[nodiscard]] auto path(char separator) const -> std::string;
        [[nodiscard]] auto depth() const -> int;
    };

    using node_ptr = std::shared_ptr<node>;

    using select_handler = std::function<void(const node_ptr &node, const std::string &slash, const std::string &dot)>;

    [[nodiscard]] auto get_root() const -> const node_ptr & { return root_; }
    [[nodiscard]] auto get_selected_node() const -> node_ptr { return selected_.lock(); }

    void clear_nodes();
    auto add_node(const std::string &name) -> node_ptr;
    auto add_node_by_path(const std::string &path, const std::string &separator) -> node_ptr;
    [[nodiscard]] auto get_node_by_path(const std::string &path, const std::string &separator) const -> node_ptr;
    [[nodiscard]] auto get_node_at(Vector2 pt) const -> node_ptr;
    void sort();

    void set_text_profile(const std::shared_ptr<gui_control_profile> &profile) { text_profile_ = profile; }
    void set_columns(std::vector<float> columns) { columns_ = std::move(columns); }

    select_handler selected;

    void draw() const override;

  protected:
    void on_mouse_pressed(int button, Vector2 pt) override;

  private:
    struct visible_row {
        node_ptr node;
        int depth = 0;
    };

    [[nodiscard]] auto rows() const -> std::vector<visible_row>;

    void fit_height();

    [[nodiscard]] auto row_height() const -> float;
    [[nodiscard]] auto text_profile() const -> const std::shared_ptr<gui_control_profile> &;

    node_ptr root_ = std::make_shared<node>();
    std::weak_ptr<node> selected_;
    std::shared_ptr<gui_control_profile> text_profile_;
    std::vector<float> columns_;
};
