#pragma once

#include "actor.hpp"

#include <net/message.hpp>

class remote_player final : public actor {
  public:
    remote_player() = default;

    explicit remote_player(const og::net::player_state &state);

    void apply(const og::net::player_state &state);
    void update(float dt) override;
    void draw() const override;

    void say(std::string text);

    [[nodiscard]] auto get_nickname() const -> const std::string & { return nickname_; }

  private:
    std::string nickname_;
    std::string chat_;
    float chat_timer_ = 0.0f;
    Vector2 target_{};
};
