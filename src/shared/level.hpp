#pragma once

#include <shared/result.hpp>

#include <cstdint>
#include <filesystem>
#include <istream>
#include <string>
#include <vector>

namespace og::shared {
    inline constexpr int level_width = 64;
    inline constexpr int level_height = 64;
    inline constexpr int level_tile_count = level_width * level_height;

    using tile_id = int16_t;

    struct level_layer {
        int index = 0;
        std::vector<tile_id> tiles;
    };

    struct level_link {
        std::string destination;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        std::string destination_x;
        std::string destination_y;
    };

    struct level_sign {
        int x = 0;
        int y = 0;
        std::string text;
    };

    struct level_chest {
        int x = 0;
        int y = 0;
        std::string item;
        int sign_index = 0;
    };

    struct level_baddy {
        int x = 0;
        int y = 0;
        int type = 0;
        std::vector<std::string> verses;
    };

    struct level_npc {
        std::string image;
        float x = 0.0f;
        float y = 0.0f;
        std::string script;
    };

    struct level_data {
        std::string name;
        std::string version;
        std::vector<level_layer> layers;
        std::vector<level_link> links;
        std::vector<level_sign> signs;
        std::vector<level_chest> chests;
        std::vector<level_baddy> baddies;
        std::vector<level_npc> npcs;
        std::vector<std::string> raw_lines;

        [[nodiscard]] auto find_layer(int index) const -> const level_layer *;
        [[nodiscard]] auto tiles() const -> const std::vector<tile_id> *;
    };

    auto load_level(const std::filesystem::path &path) -> result<level_data>;
    auto load_level(std::string name, std::istream &is) -> result<level_data>;

    auto save_level(const level_data &level, std::ostream &os) -> void;

    [[nodiscard]]
    auto save_level_text(const level_data &level) -> std::string;
}
