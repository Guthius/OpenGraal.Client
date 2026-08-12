#pragma once

#include <raylib.h>
#include <string>

namespace dev_input {
    auto load(const std::string &path) -> bool;

    [[nodiscard]] auto is_active() -> bool;

    auto update(float dt) -> bool;

    [[nodiscard]] auto key_down(int key) -> bool;
    [[nodiscard]] auto key_pressed(int key) -> bool;

    auto char_pressed() -> int;

    auto take_login(std::string &account, std::string &password) -> bool;
    [[nodiscard]] auto mouse_position() -> Vector2;
    [[nodiscard]] auto mouse_down(int button) -> bool;
    [[nodiscard]] auto mouse_pressed(int button) -> bool;
    [[nodiscard]] auto mouse_released(int button) -> bool;

    [[nodiscard]] auto mouse_wheel() -> float;
}
