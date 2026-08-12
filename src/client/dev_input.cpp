#include "dev_input.hpp"

#include "screenshot.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace {
    struct action {
        float at = 0.0f;
        std::string kind;
        std::string text;
        std::string secret;
        int key = 0;
        float hold = 0.0f;
        Vector2 point{};
        Vector2 to{};
        int button = MOUSE_BUTTON_LEFT;
        float amount = 0.0f;
        bool done = false;
    };

    std::vector<action> script;
    std::vector<int> held;
    std::vector<int> pressed_now;
    std::vector<int> typed;
    std::size_t typed_read = 0;
    std::string login_account;
    std::string login_password;
    bool login_pending = false;
    Vector2 cursor{};
    bool cursor_placed = false;
    float elapsed = 0.0f;
    int mouse_state = 0;
    int mouse_button = MOUSE_BUTTON_LEFT;
    float wheel_now = 0.0f;
    bool loaded = false;

    auto key_for(const std::string &name) -> int {
        static const std::unordered_map<std::string, int> names{
            {"up",        KEY_UP          },
            {"down",      KEY_DOWN        },
            {"left",      KEY_LEFT        },
            {"right",     KEY_RIGHT       },
            {"tab",       KEY_TAB         },
            {"enter",     KEY_ENTER       },
            {"escape",    KEY_ESCAPE      },
            {"space",     KEY_SPACE       },
            {"alt",       KEY_LEFT_ALT    },
            {"ctrl",      KEY_LEFT_CONTROL},
            {"shift",     KEY_LEFT_SHIFT  },
            {"backspace", KEY_BACKSPACE   },
            {"delete",    KEY_DELETE      },
            {"home",      KEY_HOME        },
            {"end",       KEY_END         },
            {"pageup",    KEY_PAGE_UP     },
            {"pagedown",  KEY_PAGE_DOWN   },
        };

        if (const auto match = names.find(name); match != names.end()) {
            return match->second;
        }

        if (name.size() >= 2 && name[0] == 'f') {
            const auto number = std::atoi(name.c_str() + 1);
            if (number >= 1 && number <= 12) {
                return KEY_F1 + (number - 1);
            }
        }

        if (name.size() == 1 && name[0] >= '0' && name[0] <= '9') {
            return KEY_ZERO + (name[0] - '0');
        }

        return name.size() == 1 && name[0] >= 'a' && name[0] <= 'z' ? KEY_A + (name[0] - 'a') : KEY_NULL;
    }

    auto button_for(const std::string &name) -> int {
        return name == "right" ? MOUSE_BUTTON_RIGHT : name == "middle" ? MOUSE_BUTTON_MIDDLE
                                                                       : MOUSE_BUTTON_LEFT;
    }
}

auto dev_input::load(const std::string &path) -> bool {
    std::ifstream is(path);
    if (!is.is_open()) {
        return false;
    }

    for (std::string line; std::getline(is, line);) {
        if (const auto comment = line.find('#'); comment != std::string::npos) {
            line.erase(comment);
        }

        std::istringstream fields(line);
        action next;

        if (!(fields >> next.at >> next.kind)) {
            continue;
        }

        if (next.kind == "key") {
            std::string name;
            fields >> name >> next.hold;
            next.key = key_for(name);
        } else if (next.kind == "click" || next.kind == "move") {
            std::string name;
            fields >> next.point.x >> next.point.y >> name;
            next.button = button_for(name);
        } else if (next.kind == "drag") {
            fields >> next.point.x >> next.point.y >> next.to.x >> next.to.y;

            for (std::string extra; fields >> extra;) {
                if (extra.find_first_not_of("0123456789.") == std::string::npos) {
                    next.hold = std::stof(extra);
                } else {
                    next.button = button_for(extra);
                }
            }
        } else if (next.kind == "wheel") {
            fields >> next.amount;
        } else if (next.kind == "login") {
            fields >> next.text >> next.secret;
        } else if (next.kind == "type") {
            std::getline(fields, next.text);

            const auto start = next.text.find_first_not_of(" \t");
            next.text = start == std::string::npos ? std::string{} : next.text.substr(start);
        } else {
            fields >> next.text;
        }

        script.push_back(std::move(next));
    }

    std::ranges::stable_sort(script, {}, &action::at);
    loaded = !script.empty();

    return loaded;
}

auto dev_input::is_active() -> bool {
    return loaded;
}

auto dev_input::update(const float dt) -> bool {
    if (!loaded) {
        return false;
    }

    elapsed += dt;

    pressed_now.clear();
    held.clear();
    typed.clear();
    typed_read = 0;

    mouse_state = mouse_state == 1 || mouse_state == 3 ? 2 : 0;
    wheel_now = 0.0f;

    auto stop = false;

    for (auto &next : script) {
        if (next.kind == "drag") {
            const auto hold = next.hold > 0.0f ? next.hold : 0.5f;

            if (elapsed < next.at || elapsed > next.at + hold) {
                continue;
            }

            const auto progress = std::clamp((elapsed - next.at) / hold, 0.0f, 1.0f);

            cursor = {
                next.point.x + ((next.to.x - next.point.x) * progress),
                next.point.y + ((next.to.y - next.point.y) * progress),
            };
            cursor_placed = true;

            mouse_state = next.done ? 3 : 1;
            mouse_button = next.button;
            next.done = true;

            continue;
        }

        if (next.kind == "key") {
            if (elapsed >= next.at && elapsed < next.at + next.hold) {
                held.push_back(next.key);

                if (!next.done) {
                    next.done = true;
                    pressed_now.push_back(next.key);
                }
            }

            continue;
        }

        if (next.done || elapsed < next.at) {
            continue;
        }

        next.done = true;

        if (next.kind == "click") {
            cursor = next.point;
            cursor_placed = true;
            mouse_state = 1;
            mouse_button = next.button;
        } else if (next.kind == "move") {
            cursor = next.point;
            cursor_placed = true;
        } else if (next.kind == "drag_end") {
            mouse_state = 2;
        } else if (next.kind == "wheel") {
            wheel_now += next.amount;
        } else if (next.kind == "type") {
            typed.insert(typed.end(), next.text.begin(), next.text.end());
        } else if (next.kind == "login") {
            login_account = next.text;
            login_password = next.secret;
            login_pending = true;
        } else if (next.kind == "shot") {
            capture_screenshot(next.text);
        } else if (next.kind == "quit") {
            stop = true;
        }
    }

    return stop;
}

auto dev_input::key_down(const int key) -> bool {
    return IsKeyDown(key) || std::ranges::find(held, key) != held.end();
}

auto dev_input::key_pressed(const int key) -> bool {
    return IsKeyPressed(key) || std::ranges::find(pressed_now, key) != pressed_now.end();
}

auto dev_input::char_pressed() -> int {
    return typed_read < typed.size() ? typed[typed_read++] : GetCharPressed();
}

auto dev_input::take_login(std::string &account, std::string &password) -> bool {
    if (!login_pending) {
        return false;
    }

    account = login_account;
    password = login_password;
    login_pending = false;

    return true;
}

auto dev_input::mouse_position() -> Vector2 {
    return loaded && cursor_placed ? cursor : GetMousePosition();
}

auto dev_input::mouse_down(const int button) -> bool {
    return IsMouseButtonDown(button) || (button == mouse_button && (mouse_state == 1 || mouse_state == 3));
}

auto dev_input::mouse_pressed(const int button) -> bool {
    return IsMouseButtonPressed(button) || (button == mouse_button && mouse_state == 1);
}

auto dev_input::mouse_released(const int button) -> bool {
    return IsMouseButtonReleased(button) || (button == mouse_button && mouse_state == 2);
}

auto dev_input::mouse_wheel() -> float {
    return GetMouseWheelMove() + wheel_now;
}
