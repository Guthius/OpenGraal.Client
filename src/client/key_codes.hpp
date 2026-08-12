#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace key_codes {
    [[nodiscard]] auto to_raylib(int keycode) -> int;
    [[nodiscard]] auto from_raylib(int key) -> int;
    [[nodiscard]] auto of_name(std::string_view name) -> int;
    [[nodiscard]] auto name_of(int keycode) -> std::string;
    [[nodiscard]] auto pressed_this_frame() -> std::vector<int>;
}
