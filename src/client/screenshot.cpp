#include "screenshot.hpp"

#include <raylib.h>
#include <rlgl.h>

void capture_screenshot(const std::string &path) {
    rlDrawRenderBatchActive();

    const auto image = LoadImageFromScreen();

    ExportImage(image, path.c_str());
    UnloadImage(image);
}

auto update_screenshot(const screenshot_settings *settings, const float dt) -> bool {
    if (settings == nullptr) {
        return false;
    }

    static auto elapsed = 0.0f;

    elapsed += dt;
    if (elapsed < settings->delay) {
        return false;
    }

    capture_screenshot(settings->path);

    return settings->quit_after;
}
