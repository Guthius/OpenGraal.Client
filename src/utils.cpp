#include "utils.hpp"

#include <cstdint>
#include <sstream>

namespace {
    constexpr bool is_pure_red(const Color &color) {
        return color.r == 255 && color.g == 0 && color.b == 0 && color.a == 255;
    }
}

auto split_string(const std::string &str) -> std::vector<std::string> {
    std::vector<std::string> tokens;

    std::string buf;
    std::stringstream ss(str);

    while (std::getline(ss, buf, ' ')) {
        if (buf.empty()) {
            continue;
        }

        tokens.push_back(buf);
    }

    return tokens;
}

auto find_sprite_rects(const Texture2D &texture) -> std::vector<Rectangle> {
    std::vector<Rectangle> rects;

    if (!IsTextureValid(texture) || texture.width <= 0 || texture.height <= 0) {
        return rects;
    }

    const auto image = LoadImageFromTexture(texture);
    if (image.data == nullptr || image.width <= 0 || image.height <= 0) {
        return rects;
    }

    const auto colors = LoadImageColors(image);
    if (colors == nullptr) {
        UnloadImage(image);

        return rects;
    }

    const auto w = image.width;
    const auto h = image.height;

    auto get_index = [w](const int x, const int y) { return y * w + x; };

    std::vector<uint8_t> visited(static_cast<size_t>(w) * static_cast<size_t>(h), 0);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int i = get_index(x, y);
            if (visited[i]) {
                continue;
            }

            if (is_pure_red(colors[i])) {
                visited[i] = 1;
                continue;
            }

            int max_x = x;
            while (max_x < w && !is_pure_red(colors[get_index(max_x, y)])) {
                ++max_x;
            }

            int rect_w = max_x - x;
            if (rect_w <= 0) {
                rect_w = 1;
            }

            int max_y = y;
            bool row_ok = true;
            while (max_y < h && row_ok) {
                for (int cx = x; cx < x + rect_w; ++cx) {
                    if (is_pure_red(colors[get_index(cx, max_y)])) {
                        row_ok = false;
                        break;
                    }
                }

                if (row_ok) {
                    ++max_y;
                }
            }

            int rect_h = max_y - y;
            if (rect_h <= 0) {
                rect_h = 1;
            }

            for (int yy = y; yy < y + rect_h; ++yy) {
                for (int xx = x; xx < x + rect_w; ++xx) {
                    visited[get_index(xx, yy)] = 1;
                }
            }

            rects.push_back(Rectangle{static_cast<float>(x), static_cast<float>(y), static_cast<float>(rect_w), static_cast<float>(rect_h)});
        }
    }

    UnloadImageColors(colors);
    UnloadImage(image);

    return rects;
}
