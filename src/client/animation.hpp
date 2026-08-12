#pragma once

#include "constants.hpp"

#include <array>
#include <filesystem>
#include <map>
#include <raylib.h>
#include <string>
#include <vector>

class animation;

inline constexpr size_t gani_attr_count = 31;

struct animation_state {
    size_t current_frame;
    float current_frame_duration;
    std::string body{"body.png"};
    std::string head{"head0.png"};
    std::string sword{"sword1.png"};
    std::string shield{"shield1.png"};
    std::array<std::string, gani_attr_count> attr{};
    bool ended;

    void reset(size_t frame, const animation *animation);
};

enum class sprite_source {
    file,
    sprites,
    shield,
    sword,
    head,
    body,
    attribute
};

class animation {
    struct sprite {
        int id;
        sprite_source source;
        int source_index;
        std::string texture;
        Rectangle texture_rect;
    };

    struct sprite_ref {
        sprite *sprite;
        Vector2 position;
    };

    struct frame {
        std::vector<sprite_ref> sprites[4]{};
        float duration;
        std::string play_sound;
        Vector2 play_sound_at;
    };

  public:
    animation() = default;

    void load_file(const std::filesystem::path &path);
    void update(float dt, animation_state &state) const;
    void draw(float x, float y, direction direction, const animation_state &state) const;
    void play_sound(size_t frame) const;

    [[nodiscard]]
    auto frame_count() const -> size_t { return frames_.size(); }

    [[nodiscard]]
    auto frame_duration(const size_t frame) const -> float { return frames_[frame].duration; }

  private:
    void draw_sprites(const animation_state &state, const std::vector<sprite_ref> &sprite_refs) const;

    [[nodiscard]] auto get_texture_filename(const animation_state &state, const sprite_ref &sprite_ref) const -> std::string;

    std::string set_back_to_;
    std::array<std::string, gani_attr_count> default_attrs_{};
    std::string default_head_{"head19.png"};
    std::string default_body_{"body.png"};
    bool single_direction_ = false;
    bool continuous_ = false;
    std::map<int, sprite> sprites_;
    std::vector<frame> frames_;
};
