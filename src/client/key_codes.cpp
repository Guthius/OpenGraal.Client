#include "key_codes.hpp"

#include "dev_input.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>

namespace {
    struct key_name {
        int code;
        int key;
        std::string_view name;
    };

    constexpr std::array specials = {
        key_name{.code = 8,   .key = KEY_BACKSPACE,     .name = "backspace"},
        key_name{.code = 9,   .key = KEY_TAB,           .name = "tab"      },
        key_name{.code = 13,  .key = KEY_ENTER,         .name = "enter"    },
        key_name{.code = 16,  .key = KEY_LEFT_SHIFT,    .name = "shift"    },
        key_name{.code = 17,  .key = KEY_LEFT_CONTROL,  .name = "ctrl"     },
        key_name{.code = 18,  .key = KEY_LEFT_ALT,      .name = "alt"      },
        key_name{.code = 27,  .key = KEY_ESCAPE,        .name = "escape"   },
        key_name{.code = 32,  .key = KEY_SPACE,         .name = "space"    },
        key_name{.code = 33,  .key = KEY_PAGE_UP,       .name = "pageup"   },
        key_name{.code = 34,  .key = KEY_PAGE_DOWN,     .name = "pagedown" },
        key_name{.code = 35,  .key = KEY_END,           .name = "end"      },
        key_name{.code = 36,  .key = KEY_HOME,          .name = "home"     },
        key_name{.code = 37,  .key = KEY_LEFT,          .name = "left"     },
        key_name{.code = 38,  .key = KEY_UP,            .name = "up"       },
        key_name{.code = 39,  .key = KEY_RIGHT,         .name = "right"    },
        key_name{.code = 40,  .key = KEY_DOWN,          .name = "down"     },
        key_name{.code = 45,  .key = KEY_INSERT,        .name = "insert"   },
        key_name{.code = 46,  .key = KEY_DELETE,        .name = "delete"   },
        key_name{.code = 160, .key = KEY_LEFT_SHIFT,    .name = "lshift"   },
        key_name{.code = 161, .key = KEY_RIGHT_SHIFT,   .name = "rshift"   },
        key_name{.code = 162, .key = KEY_LEFT_CONTROL,  .name = "lctrl"    },
        key_name{.code = 163, .key = KEY_RIGHT_CONTROL, .name = "rctrl"    },
        key_name{.code = 164, .key = KEY_LEFT_ALT,      .name = "lalt"     },
        key_name{.code = 165, .key = KEY_RIGHT_ALT,     .name = "ralt"     },
    };

    constexpr int first_function_code = 112;
    constexpr int function_key_count = 12;

    auto is_letter_or_digit(const int code) -> bool {
        return (code >= '0' && code <= '9') || (code >= 'A' && code <= 'Z');
    }
}

namespace key_codes {
    auto to_raylib(const int keycode) -> int {
        if (is_letter_or_digit(keycode)) {
            return keycode;
        }

        if (keycode >= first_function_code && keycode < first_function_code + function_key_count) {
            return KEY_F1 + (keycode - first_function_code);
        }

        const auto *const match = std::ranges::find(specials, keycode, &key_name::code);

        return match == specials.end() ? 0 : match->key;
    }

    auto from_raylib(const int key) -> int {
        if (is_letter_or_digit(key)) {
            return key;
        }

        if (key >= KEY_F1 && key < KEY_F1 + function_key_count) {
            return first_function_code + (key - KEY_F1);
        }

        const auto *const match = std::ranges::find(specials, key, &key_name::key);

        return match == specials.end() ? 0 : match->code;
    }

    auto of_name(const std::string_view name) -> int {
        if (name.size() == 1) {
            return std::toupper(static_cast<unsigned char>(name.front()));
        }

        std::string folded;
        folded.reserve(name.size());

        for (const auto ch : name) {
            folded.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }

        if (folded.size() >= 2 && folded.front() == 'f') {
            if (const auto index = atoi(folded.c_str() + 1); index >= 1 && index <= function_key_count) {
                return first_function_code + index - 1;
            }
        }

        const auto *const match = std::ranges::find(specials, folded, &key_name::name);

        return match == specials.end() ? 0 : match->code;
    }

    auto name_of(const int keycode) -> std::string {
        if (is_letter_or_digit(keycode)) {
            return std::string(1, static_cast<char>(std::tolower(keycode)));
        }

        if (keycode >= first_function_code && keycode < first_function_code + function_key_count) {
            return "f" + std::to_string(keycode - first_function_code + 1);
        }

        const auto *const match = std::ranges::find(specials, keycode, &key_name::code);

        return match == specials.end() ? std::string{} : std::string(match->name);
    }

    auto pressed_this_frame() -> std::vector<int> {
        std::vector<int> pressed;

        for (const auto &special : specials) {
            if (dev_input::key_pressed(special.key)) {
                pressed.push_back(special.code);
            }
        }

        for (auto code = '0'; code <= '9'; ++code) {
            if (dev_input::key_pressed(code)) {
                pressed.push_back(code);
            }
        }

        for (auto code = 'A'; code <= 'Z'; ++code) {
            if (dev_input::key_pressed(code)) {
                pressed.push_back(code);
            }
        }

        for (auto index = 0; index < function_key_count; ++index) {
            if (dev_input::key_pressed(KEY_F1 + index)) {
                pressed.push_back(first_function_code + index);
            }
        }

        return pressed;
    }
}
