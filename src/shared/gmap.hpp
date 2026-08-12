#pragma once

#include <shared/result.hpp>

#include <filesystem>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace og::shared {
    struct gmap_position {
        int x = 0;
        int y = 0;
    };

    struct gmap_data {
        std::string name;
        int width = 0;
        int height = 0;
        std::string generated;
        std::string map_image;
        std::string minimap_image;
        bool no_auto_mapping = false;
        bool load_full_map = false;
        std::vector<std::string> load_at_start;
        std::vector<std::string> level_names;
        std::vector<int> height_map;
        std::vector<uint32_t> random_seeds;
        double level_height = 0.0;
        double level_chaos = 0.0;

        [[nodiscard]] auto level_at(int x, int y) const -> std::string;
        [[nodiscard]] auto position_of(std::string_view level_name) const -> std::optional<gmap_position>;
    };

    auto load_gmap(const std::filesystem::path &path) -> result<gmap_data>;
    auto load_gmap(std::string name, std::istream &is) -> result<gmap_data>;

    auto derive_generated_level_names(std::string_view generated, int width, int height) -> std::vector<std::string>;
}
