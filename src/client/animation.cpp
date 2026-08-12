#include "animation.hpp"

#include "constants.hpp"
#include "sound_manager.hpp"
#include "texture_manager.hpp"

#include <algorithm>
#include <shared/gani.hpp>

#include <rlgl.h>

namespace {
    auto to_sprite_source(const og::shared::gani_sprite &sprite) -> sprite_source {
        switch (sprite.source) {
        case og::shared::gani_sprite_source::sprites: return sprite_source::sprites;
        case og::shared::gani_sprite_source::head:    return sprite_source::head;
        case og::shared::gani_sprite_source::body:    return sprite_source::body;
        case og::shared::gani_sprite_source::sword:   return sprite_source::sword;
        case og::shared::gani_sprite_source::shield:  return sprite_source::shield;

        case og::shared::gani_sprite_source::attribute: return sprite_source::attribute;

        default:
            return sprite_source::file;
        }
    }
}

void animation_state::reset(size_t frame, const animation *animation) {
    if (animation == nullptr) {
        return;
    }

    frame = std::min(frame, animation->frame_count() - 1);

    current_frame = frame;
    current_frame_duration = animation->frame_duration(frame);
    ended = false;

    animation->play_sound(frame);
}

void animation::load_file(const std::filesystem::path &path) {
    const auto gani = og::shared::load_gani(path);
    if (!gani) {
        TraceLog(LOG_WARNING, "ANIMATION: %s", gani.error().c_str());

        return;
    }

    single_direction_ = gani->single_direction;
    continuous_ = gani->continuous;
    set_back_to_ = gani->set_back_to;

    if (!gani->default_head.empty()) {
        default_head_ = gani->default_head;
    }

    if (!gani->default_body.empty()) {
        default_body_ = gani->default_body;
    }

    for (const auto &[index, image] : gani->default_attributes) {
        if (index > 0 && static_cast<size_t>(index) < default_attrs_.size()) {
            default_attrs_[index] = image;
        }
    }

    for (const auto &[id, source] : gani->sprites) {
        sprites_[id] = sprite{
            .id = id,
            .source = to_sprite_source(source),
            .source_index = source.source_index,
            .texture = source.image,
            .texture_rect = {
                             .x = static_cast<float>(source.x),
                             .y = static_cast<float>(source.y),
                             .width = static_cast<float>(source.width),
                             .height = static_cast<float>(source.height),
                             },
        };
    }

    frames_.reserve(gani->frames.size());
    for (const auto &source : gani->frames) {
        frame frame{};

        frame.duration = source.duration;
        frame.play_sound = source.sound;
        frame.play_sound_at = {source.sound_x, source.sound_y};

        for (size_t i = 0; i < source.directions.size(); ++i) {
            for (const auto &ref : source.directions[i]) {
                const auto match = sprites_.find(ref.sprite_id);
                if (match == sprites_.end()) {
                    continue;
                }

                frame.sprites[i].push_back({
                    .sprite = &match->second,
                    .position = {ref.x, ref.y},
                });
            }
        }

        frames_.push_back(std::move(frame));
    }
}

void animation::update(const float dt, animation_state &state) const {
    if (state.current_frame < 0 || state.current_frame >= frames_.size()) {
        state.current_frame = 0;
        state.current_frame_duration = frames_[0].duration;
    }

    state.current_frame_duration -= dt;

    if (state.current_frame_duration > 0) {
        return;
    }

    if (state.current_frame < frames_.size() - 1) {
        state.current_frame++;
        state.current_frame_duration = frames_[state.current_frame].duration;

        const auto &sound = frames_[state.current_frame].play_sound;
        if (!sound.empty()) {
            ::play_sound(sound);
        }

        return;
    }

    if (continuous_) {
        state.current_frame = 0;
        state.current_frame_duration = frames_[0].duration;

        const auto &sound = frames_[state.current_frame].play_sound;
        if (!sound.empty()) {
            ::play_sound(sound);
        }

        return;
    }

    state.ended = true;
}

void animation::draw(const float x, const float y, direction direction, const animation_state &state) const {
    if (frames_.empty()) {
        return;
    }

    if (single_direction_) {
        direction = direction::up;
    }

    auto frame_index = std::min(state.current_frame, frames_.size() - 1);

    const auto &frame = frames_[frame_index];
    const auto &sprites = frame.sprites[static_cast<int>(direction)];

    if (sprites.empty()) {
        return;
    }

    rlPushMatrix();
    rlTranslatef(x, y, 0);

    draw_sprites(state, sprites);

    rlPopMatrix();
}

void animation::play_sound(const size_t frame) const {
    if (const auto &sound = frames_[frame].play_sound; !sound.empty()) {
        ::play_sound(sound);
    }
}

void animation::draw_sprites(const animation_state &state, const std::vector<sprite_ref> &sprite_refs) const {
    for (const auto &sprite_ref : sprite_refs) {
        if (sprite_ref.sprite == nullptr) {
            continue;
        }

        auto texture_name = get_texture_filename(state, sprite_ref);
        if (texture_name.empty()) {
            continue;
        }

        const auto texture = load_texture(texture_name);

        if (!IsTextureValid(texture)) {
            continue;
        }

        DrawTextureRec(
            texture,
            sprite_ref.sprite->texture_rect,
            sprite_ref.position,
            WHITE);
    }
}

std::string animation::get_texture_filename(const animation_state &state, const sprite_ref &sprite_ref) const {
    switch (sprite_ref.sprite->source) {
    case sprite_source::file:    return sprite_ref.sprite->texture;
    case sprite_source::sprites: return "sprites.png";
    case sprite_source::shield:  return state.shield;
    case sprite_source::sword:   return state.sword;

    case sprite_source::head:
        {
            if (state.head.empty()) {
                return default_head_;
            }

            return state.head;
        }

    case sprite_source::body:
        {
            if (state.body.empty()) {
                return default_body_;
            }

            return state.body;
        }

    case sprite_source::attribute:
        {
            const auto index = static_cast<size_t>(sprite_ref.sprite->source_index);
            if (index >= state.attr.size()) {
                return {};
            }

            return state.attr[index].empty() ? default_attrs_[index] : state.attr[index];
        }
    }

    return {};
}
