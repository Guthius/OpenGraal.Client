#pragma once

#include <shared/result.hpp>

#include <array>
#include <filesystem>
#include <istream>
#include <map>
#include <string>
#include <vector>

namespace og::shared {
    inline constexpr int gani_direction_count = 4;
    inline constexpr int gani_attribute_count = 30;
    inline constexpr float gani_default_frame_duration = 0.06f;

    enum class gani_sprite_source {
        file,
        sprites,
        head,
        body,
        sword,
        shield,
        horse,
        attribute,
        parameter
    };

    struct gani_sprite {
        int id = 0;
        gani_sprite_source source = gani_sprite_source::file;
        int source_index = 0;
        std::string image;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        std::string description;
    };

    struct gani_sprite_ref {
        int sprite_id = 0;
        float x = 0.0f;
        float y = 0.0f;
    };

    struct gani_attachment {
        int sprite_id = 0;
        int attached_sprite_id = 0;
        float x = 0.0f;
        float y = 0.0f;
    };

    struct gani_frame {
        std::array<std::vector<gani_sprite_ref>, gani_direction_count> directions;
        float duration = gani_default_frame_duration;
        std::string sound;
        float sound_x = 0.0f;
        float sound_y = 0.0f;
    };

    struct gani_data {
        std::string name;
        std::map<int, gani_sprite> sprites;
        std::vector<gani_frame> frames;
        std::vector<gani_attachment> attachments;
        std::map<int, std::string> default_attributes;
        std::string default_head;
        std::string default_body;
        std::string set_back_to;
        bool single_direction = false;
        bool continuous = false;
        bool loop = false;
        std::string script;

        [[nodiscard]] auto find_sprite(int id) const -> const gani_sprite *;
    };

    auto load_gani(const std::filesystem::path &path) -> result<gani_data>;
    auto load_gani(std::string name, std::istream &is) -> result<gani_data>;
}
