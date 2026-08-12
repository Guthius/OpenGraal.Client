#include "remote_player.hpp"

#include "constants.hpp"
#include "font_manager.hpp"

#include <raylib.h>
#include <raymath.h>
#include <utility>

namespace {
    constexpr float interpolation_speed = 12.0f;
    constexpr float chat_duration = 5.0f;
    constexpr float name_offset_y = -12.0f;
    constexpr float chat_offset_y = -26.0f;
}

remote_player::remote_player(const og::net::player_state &state) {
    apply(state);

    set_position(target_);
}

void remote_player::apply(const og::net::player_state &state) {
    target_ = {state.x * tile_size, state.y * tile_size};

    set_direction(static_cast<direction>(state.direction % 4));
    set_animation(state.animation.empty() ? "idle" : state.animation);
    set_appearance(state.head, state.body, state.attr);

    nickname_ = state.nickname;
}

void remote_player::update(const float dt) {
    set_position(Vector2Lerp(get_position(), target_, std::min(1.0f, dt * interpolation_speed)));

    if (chat_timer_ > 0.0f) {
        chat_timer_ -= dt;

        if (chat_timer_ <= 0.0f) {
            chat_.clear();
        }
    }

    actor::update(dt);
}

void remote_player::draw() const {
    actor::draw();

    const auto [x, y] = get_position();
    const auto font = load_font("LiberationSans-Regular.ttf", 12);

    if (!nickname_.empty()) {
        const auto width = MeasureTextEx(font, nickname_.c_str(), 12, 1).x;

        DrawTextEx(font, nickname_.c_str(), {x + 16 - (width / 2), y + name_offset_y}, 12, 1, WHITE);
    }

    if (!chat_.empty()) {
        const auto width = MeasureTextEx(font, chat_.c_str(), 12, 1).x;
        const auto position = Vector2{x + 16 - (width / 2), y + chat_offset_y};

        DrawRectangle(
            static_cast<int>(position.x) - 3,
            static_cast<int>(position.y) - 2,
            static_cast<int>(width) + 6,
            16,
            Fade(BLACK, 0.55f));

        DrawTextEx(font, chat_.c_str(), position, 12, 1, WHITE);
    }
}

void remote_player::say(std::string text) {
    chat_ = std::move(text);
    chat_timer_ = chat_duration;
}
