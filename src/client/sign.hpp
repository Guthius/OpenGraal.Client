#pragma once

#include <raylib.h>
#include <string>
#include <vector>

class sign {
  public:
    sign();

    [[nodiscard]] auto is_open() const -> bool { return open_; }

    void show(const std::string &str);
    void draw(float width, float height) const;
    void update();

  private:
    Texture2D texture_{};
    bool open_ = false;
    std::vector<std::string> pages_;
    int page_ = 0;
    Sound next_page_sound_;
};
