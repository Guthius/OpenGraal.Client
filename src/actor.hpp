#pragma once

#include "animation.hpp"
#include "animation_manager.hpp"
#include "carry_object.hpp"
#include "constants.hpp"

#include <raylib.h>
#include <string>

enum class terrain_effect_type {
    none,
    swamp,
    lava_swamp,
    near_water,
    near_lava
};

class game;

class actor {
  public:
    explicit actor();

    virtual ~actor() = default;

    virtual void update(float dt);
    virtual void draw() const;

    [[nodiscard]] auto get_position() const -> Vector2 { return position_; }
    [[nodiscard]] auto get_direction() const -> direction { return dir_; }
    [[nodiscard]] auto get_animation() const -> const std::string & { return animation_name_; }
    [[nodiscard]] auto get_animation_state() const -> const animation_state & { return animation_state_; }
    [[nodiscard]] auto get_carried_object() const -> carry_object_type { return carried_object_; }
    [[nodiscard]] auto is_carrying() const -> bool { return carried_object_ != carry_object_type::none; }
    [[nodiscard]] auto get_terrain() const -> terrain_effect_type { return terrain_effect_type_; }
    [[nodiscard]] auto get_velocity() const -> Vector2 { return velocity_; }
    [[nodiscard]] auto is_facing_wall() const -> bool;
    [[nodiscard]] auto get_terrain_tile_type() const -> int { return terrain_tile_type_; }

    void set_position(const Vector2 &position) { position_ = position; }
    void set_direction(const direction dir) { dir_ = dir; }
    void set_animation(const std::string &name);
    void set_carried_object(carry_object_type type);
    void set_terrain(terrain_effect_type type);
    void set_velocity(const Vector2 velocity) { velocity_ = velocity; }

  protected:
    virtual auto get_carried_destination_override(Vector2 &dest) const -> bool { return false; }

    [[nodiscard]] auto get_facing_walls(direction dir) const -> std::tuple<bool, bool>;
    [[nodiscard]] auto look_at(direction dir) const -> Vector2;

    auto move(float dt) -> bool;
    bool slide(float dt);
    void update_terrain();

  private:
    void draw_terrain() const;

    Texture2D sprites_;
    Vector2 position_{};
    direction dir_ = direction::up;
    animation_state animation_state_{};
    std::string animation_name_{};
    animation *animation_ = nullptr;
    carry_object_type carried_object_ = carry_object_type::none;
    Rectangle carried_object_rectangle_{};
    terrain_effect_type terrain_effect_type_ = terrain_effect_type::none;
    int terrain_tile_type_{};
    Rectangle terrain_effect_rect_{};
    int terrain_effect_frame_ = 0;
    float terrain_effect_timer_ = 0.1f;
    Vector2 velocity_{};
};
