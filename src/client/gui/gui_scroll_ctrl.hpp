#pragma once

#include "gui_control.hpp"

#include <string>

class gui_scroll_ctrl : public gui_control {
  public:
    [[nodiscard]] auto get_h_scroll_bar() const -> const std::string & { return h_scroll_bar_; }
    [[nodiscard]] auto get_v_scroll_bar() const -> const std::string & { return v_scroll_bar_; }
    [[nodiscard]] auto is_tiled() const -> bool { return tile_; }

    void set_h_scroll_bar(std::string mode) { h_scroll_bar_ = std::move(mode); }
    void set_v_scroll_bar(std::string mode) { v_scroll_bar_ = std::move(mode); }
    void set_tiled(const bool tile) { tile_ = tile; }

    void draw() const override;

  private:
    std::string h_scroll_bar_ = "dynamic";
    std::string v_scroll_bar_ = "dynamic";
    bool tile_ = false;
};
