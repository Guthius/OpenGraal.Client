#include "font_manager.hpp"

#include "file_manager.hpp"

#include <map>
#include <boost/algorithm/string.hpp>

namespace {
    std::map<std::string, Font> loaded_fonts;

    auto load_font_from_file(const std::string &filename, const float font_size, const std::string &key) -> Font {
        const auto path = find_file(filename);

        if (path.empty()) {
            return loaded_fonts[key] = {};
        }

        const auto font = LoadFontEx(path.string().c_str(), static_cast<int>(font_size), nullptr, 0);

        return loaded_fonts[key] = font;
    }
}

auto load_font(const std::string &filename, const float font_size) -> Font {
    const auto key = boost::to_lower_copy(filename) + "|" + std::to_string(static_cast<int>(font_size));
    const auto iter = loaded_fonts.find(key);

    if (iter == loaded_fonts.end()) {
        return load_font_from_file(filename, font_size, key);
    }

    return iter->second;
}
