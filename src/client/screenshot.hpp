#pragma once

#include <string>

struct screenshot_settings {
    std::string path;
    float delay = 3.0f;
    bool quit_after = true;
};

void capture_screenshot(const std::string &path);

[[nodiscard]]
auto update_screenshot(const screenshot_settings *settings, float dt) -> bool;
