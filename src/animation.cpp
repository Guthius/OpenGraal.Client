#include "animation.hpp"

#include "constants.hpp"
#include "sound_manager.hpp"
#include "texture_manager.hpp"
#include "utils.hpp"

#include <boost/algorithm/string.hpp>
#include <fstream>
#include <rlgl.h>

namespace {
    sprite_source parse_sprite_source(const std::string &str) {
        if (str == "SPRITES") return sprite_source::sprites;
        if (str == "SHIELD") return sprite_source::shield;
        if (str == "SWORD") return sprite_source::sword;
        if (str == "HEAD") return sprite_source::head;
        if (str == "BODY") return sprite_source::body;
        if (str == "ATTR1") return sprite_source::attr1;

        return sprite_source::file;
    }
}

void animation_state::reset(size_t frame, const animation *animation) {
    if (animation == nullptr) {
        return;
    }

    if (const auto max_frame = animation->frame_count() - 1; frame > max_frame) {
        frame = max_frame;
    }

    current_frame = frame;
    current_frame_duration = animation->frame_duration(frame);
    ended = false;

    animation->play_sound(frame);
}

void animation::parse_sprite(const std::vector<std::string> &tokens) {
    if (tokens.size() < 7) {
        return;
    }

    sprite sprite;

    sprite.id = std::stoi(tokens[1]);
    sprite.source = parse_sprite_source(tokens[2]);
    sprite.texture_rect = {
        .x = std::stof(tokens[3]),
        .y = std::stof(tokens[4]),
        .width = std::stof(tokens[5]),
        .height = std::stof(tokens[6])};

    if (sprite.source == sprite_source::file) {
        sprite.texture = tokens[2];
    }

    sprites_[sprite.id] = sprite;
}

void animation::parse_ani(std::ifstream &stream) {
    std::string line;
    while (std::getline(stream, line)) {
        boost::trim(line);

        if (line == "ANIEND") {
            return;
        }

        frame frame{};

        frame.duration = 0.06f;

        if (single_direction_) {
            parse_sprites(line, frame.sprites[0]);
        } else {
            parse_sprites(line, frame.sprites[0]);

            if (!std::getline(stream, line)) break;
            parse_sprites(line, frame.sprites[1]);

            if (!std::getline(stream, line)) break;
            parse_sprites(line, frame.sprites[2]);

            if (!std::getline(stream, line)) break;
            parse_sprites(line, frame.sprites[3]);
        }

        while (std::getline(stream, line)) {
            boost::trim(line);

            if (line == "ANIEND") {
                frames_.push_back(frame);
                return;
            }

            if (line.empty()) {
                break;
            }

            auto tokens = split_string(line);
            if (tokens.empty()) {
                break;
            }

            if (tokens[0] == "WAIT" && tokens.size() == 2) {
                frame.duration = std::stof(tokens[1]) / 10;
            } else if (tokens[0] == "PLAYSOUND" && tokens.size() == 4) {
                frame.play_sound = tokens[1];
                frame.play_sound_at = {
                    .x = std::stof(tokens[2]),
                    .y = std::stof(tokens[3])};
            }
        }

        frames_.push_back(frame);
    }
}

void animation::parse_sprites(std::string &line, std::vector<sprite_ref> &frame) {
    std::vector<std::string> sprite_infos;
    std::vector<std::string> tokens;

    boost::trim(line);

    if (line.empty()) {
        return;
    }

    boost::split(sprite_infos, line, boost::is_any_of(","));

    for (auto &sprite_info : sprite_infos) {
        boost::trim(sprite_info);

        if (sprite_info.empty()) {
            continue;
        }

        boost::split(tokens, sprite_info, boost::is_any_of(" "), boost::token_compress_on);
        if (tokens.size() != 3) {
            continue;
        }

        auto id = std::stoi(tokens[0]);
        auto sprite = sprites_.find(id);

        if (sprite == sprites_.end()) {
            continue;
        }

        frame.push_back({
            .sprite = &sprite->second,
            .position = {
                         .x = std::stof(tokens[1]),
                         .y = std::stof(tokens[2])}
        });
    }
}

void animation::load_file(const std::filesystem::path &path) {
    if (!is_regular_file(path)) {
        return;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        boost::trim(line);

        if (line.empty()) {
            continue;
        }

        auto tokens = split_string(line);

        if (tokens.empty()) {
            continue;
        }

        if (tokens[0] == "SPRITE") {
            parse_sprite(tokens);
        } else if (tokens[0] == "SINGLEDIRECTION") {
            single_direction_ = true;
        } else if (tokens[0] == "CONTINUOUS") {
            continuous_ = true;
        } else if (tokens[0] == "SETBACKTO") {
            set_back_to_ = line.substr(10);
        } else if (tokens[0] == "DEFAULTATTR1") {
            default_attr1_ = line.substr(13);
        } else if (tokens[0] == "DEFAULTHEAD") {
            default_head_ = line.substr(12);
        } else if (tokens[0] == "DEFAULTBODY") {
            default_body_ = line.substr(12);
        } else if (tokens[0] == "ANI") {
            parse_ani(file);
        }
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

        auto &sound = frames_[state.current_frame].play_sound;
        if (!sound.empty()) {
            ::play_sound(sound);
        }

        return;
    }

    if (continuous_) {
        state.current_frame = 0;
        state.current_frame_duration = frames_[0].duration;

        auto &sound = frames_[state.current_frame].play_sound;
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

    auto frame_index = state.current_frame;
    if (frame_index > frames_.size() - 1) {
        frame_index = frames_.size() - 1;
    }

    auto &frame = frames_[frame_index];
    auto &sprites = frame.sprites[static_cast<int>(direction)];

    if (sprites.empty()) {
        return;
    }

    rlPushMatrix();
    rlTranslatef(x, y, 0);

    draw_sprites(state, sprites);

    rlPopMatrix();
}

void animation::play_sound(const size_t frame) const {
    if (auto &sound = frames_[frame].play_sound; !sound.empty()) {
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

    case sprite_source::attr1:
        return state.attr1;
    }

    return {};
}
