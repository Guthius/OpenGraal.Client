#include "input.hpp"

#include "dev_input.hpp"
#include "gui/gui.hpp"

#include <raylib.h>
#include <unordered_map>

namespace {
    const std::unordered_map<input_action, int> keybindings{
        {input_action::up,        KEY_UP   },
        {input_action::left,      KEY_LEFT },
        {input_action::down,      KEY_DOWN },
        {input_action::right,     KEY_RIGHT},
        {input_action::shoot,     KEY_D    },
        {input_action::attack,    KEY_S    },
        {input_action::grab,      KEY_A    },
        {input_action::map,       KEY_M    },
        {input_action::chat,      KEY_TAB  },
        {input_action::inventory, KEY_Q    }
    };

    auto key_for(const input_action action) -> int {
        if (const auto iter = keybindings.find(action); iter != keybindings.end()) {
            return iter->second;
        }

        return KEY_NULL;
    }
}

auto is_action_down(const input_action action) -> bool {
    return !gui_has_focus() && dev_input::key_down(key_for(action));
}

auto is_action_up(const input_action action) -> bool {
    return !gui_has_focus() && !dev_input::key_down(key_for(action));
}

auto is_action_pressed(const input_action action) -> bool {
    return !gui_has_focus() && dev_input::key_pressed(key_for(action));
}

auto is_action_released(const input_action action) -> bool {
    return !gui_has_focus() && IsKeyReleased(key_for(action));
}

auto get_input_direction_vector() -> Vector2 {
    if (gui_has_focus()) {
        return {0, 0};
    }

    const auto x = is_action_down(input_action::right) - is_action_down(input_action::left);
    const auto y = is_action_down(input_action::down) - is_action_down(input_action::up);

    return Vector2(static_cast<float>(x), static_cast<float>(y));
}
