#include "animation_manager.hpp"
#include "dev_input.hpp"
#include "game.hpp"
#include "network.hpp"
#include "screenshot.hpp"

#include <shared/text.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace {
    constexpr auto title = "OpenGraal";

    auto parse_network(const int argc, char *argv[]) -> network_settings {
        network_settings settings;

        for (int i = 1; i < argc; ++i) {
            const auto argument = std::string_view(argv[i]);

            if (argument.starts_with("--connect=")) {
                settings.host = argument.substr(10);
            } else if (argument.starts_with("--port=")) {
                settings.port = static_cast<uint16_t>(og::shared::to_int(argument.substr(7), 14900));
            }
        }

        return settings;
    }

    auto parse_screenshot(const int argc, char *argv[]) -> std::optional<screenshot_settings> {
        screenshot_settings settings;
        auto wanted = false;

        for (int i = 1; i < argc; ++i) {
            const auto argument = std::string_view(argv[i]);

            if (argument.starts_with("--screenshot=")) {
                settings.path = argument.substr(13);
                wanted = true;
            } else if (argument.starts_with("--screenshot-delay=")) {
                settings.delay = static_cast<float>(og::shared::to_int(argument.substr(19), 3));
            } else if (argument == "--screenshot-stay") {
                settings.quit_after = false;
            }
        }

        return wanted ? std::optional(std::move(settings)) : std::nullopt;
    }
}

auto main(const int argc, char *argv[]) -> int {
    const auto network = parse_network(argc, argv);
    const auto screenshot = parse_screenshot(argc, argv);

    std::string input_script;
    for (int i = 1; i < argc; ++i) {
        if (const auto argument = std::string_view(argv[i]); argument.starts_with("--input=")) {
            input_script = argument.substr(8);
        }
    }

    InitAudioDevice();

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    load_animations_from_directory("levels/ganis");

    InitWindow(1280, 720, title);

    SetExitKey(KEY_NULL);

    if (!input_script.empty() && !dev_input::load(input_script)) {
        TraceLog(LOG_ERROR, "INPUT: could not read '%s'", input_script.c_str());
    }

    run_game(&network, screenshot ? &*screenshot : nullptr);

    CloseWindow();

    return 0;
}
