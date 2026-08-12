#pragma once

#include "actor.hpp"

class game;

enum class player_state {
    idle,
    walk,
    grab,
    push,
    pull,
    swim,
    sit,
    jump,
    attack,
    lift,
    carrystill,
    carry
};

class player final : public actor {
  public:
    explicit player();

    void update(float dt) override;

    auto try_move_from_wall(Vector2 position) -> void;

  private:
    auto check_for_level_link_at_at(const Vector2 &position) -> bool;

    [[maybe_unused]]
    auto check_for_sign_at(const Vector2 &position) const -> bool;

    void try_destroy_object_facing(const Vector2 &position) const;
    auto try_pickup_item() -> bool;

    [[nodiscard]]
    auto get_tile_facing() const -> int;

    auto drop_carried_object() -> bool;

    auto check_jump(float dt, Vector2 &position) -> bool;

    [[nodiscard]]
    auto can_jump(const Vector2 &position) const -> bool;

    void jump();
    auto jump_update(float dt, Vector2 &position) -> bool;

    void state_enter(player_state state);
    auto state_update(player_state state, float dt) -> player_state;
    void state_exit(player_state state);
    auto attack() -> player_state;

    bool get_carried_destination_override(Vector2 &dest) const override;

    player_state state_ = player_state::idle;
    float walk_speed_ = 245.0f;
    Texture2D sprites_{};
    float push_timer_ = 0.0f;
    float lift_show_timer_ = 0.0f;
    float jump_timer_ = 0.0f;
    int jump_step_ = 0;
    Vector2 jump_origin_{};
    Vector2 jump_from_{};
    Vector2 jump_to_{};
    Sound jump_sound_{};
};
