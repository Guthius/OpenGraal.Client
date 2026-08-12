#pragma once

#include <shared/level.hpp>

#include <raylib.h>
#include <string>

class level_link {
  public:
    explicit level_link(const og::shared::level_link &link);

    [[nodiscard]] auto get_new_level() const -> const std::string & { return new_level_; }
    [[nodiscard]] auto get_new_x() const -> const std::string & { return new_x_; }
    [[nodiscard]] auto get_new_y() const -> const std::string & { return new_y_; }
    [[nodiscard]] auto get_rectangle() const -> const Rectangle & { return rectangle_; }

  private:
    std::string new_level_;
    std::string new_x_;
    std::string new_y_;
    Rectangle rectangle_{};
};
