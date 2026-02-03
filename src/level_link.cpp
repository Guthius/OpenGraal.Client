#include "level_link.hpp"

#include <boost/algorithm/string.hpp>

level_link::level_link(const std::string &data) {
    std::vector<std::string> tokens;

    boost::split(tokens, data, boost::is_any_of(" "));
    if (tokens.size() < 7) {
        return;
    }

    const auto offset = tokens.size() - 7;

    auto new_level = tokens[0];
    if (offset > 0) {
        for (int i = 0; i < offset; ++i) {
            new_level += " " + tokens[offset];
        }
    }

    const auto x = std::stof(tokens[offset + 1]) * 16;
    const auto y = std::stof(tokens[offset + 2]) * 16;
    const auto w = std::stof(tokens[offset + 3]) * 16;
    const auto h = std::stof(tokens[offset + 4]) * 16;

    rectangle_ = {x, y, w, h};
    new_level_ = new_level;
    new_x_ = tokens[offset + 5];
    new_y_ = tokens[offset + 6];
}
