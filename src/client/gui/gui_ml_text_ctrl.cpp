#include "gui_ml_text_ctrl.hpp"

#include "../font_manager.hpp"

#include <sstream>

namespace {
    auto strip_markup(const std::string &text) -> std::string {
        std::string plain;
        plain.reserve(text.size());

        auto depth = 0;
        for (const auto ch : text) {
            if (ch == '<') {
                ++depth;
            } else if (ch == '>') {
                depth = depth > 0 ? depth - 1 : 0;
            } else if (depth == 0) {
                plain.push_back(ch);
            }
        }

        return plain;
    }

    auto split_lines(const std::string &text) -> std::vector<std::string> {
        std::vector<std::string> lines;

        std::istringstream is(text);
        for (std::string line; std::getline(is, line);) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            lines.push_back(std::move(line));
        }

        return lines;
    }
}

auto gui_ml_text_ctrl::get_text() const -> std::string {
    std::string text;

    for (const auto &line : lines_) {
        if (!text.empty()) {
            text.push_back('\n');
        }

        text.append(line);
    }

    return text;
}

void gui_ml_text_ctrl::set_text(const std::string &text) {
    lines_ = split_lines(text);
    scroll_ = 0;
}

void gui_ml_text_ctrl::set_lines(std::vector<std::string> lines) {
    lines_ = std::move(lines);
    scroll_ = 0;
}

void gui_ml_text_ctrl::add_text(const std::string &text) {
    for (auto &line : split_lines(text)) {
        lines_.push_back(std::move(line));
    }
}

void gui_ml_text_ctrl::scroll_to_bottom() {
    scroll_ = lines_.empty() ? 0 : lines_.size() - 1;
}

auto gui_ml_text_ctrl::line_height() const -> float {
    if (line_height_ > 0.0f) {
        return line_height_;
    }

    const auto profile = get_profile();

    return (profile ? profile->get_font_size() : 12.0f) + 2.0f;
}

auto gui_ml_text_ctrl::wrap() const -> std::vector<std::string> {
    const auto plain_of = [this](const std::string &line) {
        return parse_tags_ ? strip_markup(line) : line;
    };

    const auto profile = get_profile();
    if (!profile || !word_wrap_) {
        std::vector<std::string> plain;
        plain.reserve(lines_.size());

        for (const auto &line : lines_) {
            plain.push_back(plain_of(line));
        }

        return plain;
    }

    const auto font = load_font(profile->get_font(), profile->get_font_size());
    const auto limit = get_size().x;

    const auto width_of = [&font, profile](const std::string &text) {
        return MeasureTextEx(font, text.c_str(), profile->get_font_size(), 1.0f).x;
    };

    std::vector<std::string> wrapped;

    for (const auto &source : lines_) {
        const auto line = plain_of(source);

        std::istringstream words(line);
        std::string current;

        for (std::string word; words >> word;) {
            auto candidate = current.empty() ? word : current + " " + word;
            if (!current.empty() && width_of(candidate) > limit) {
                wrapped.push_back(std::move(current));
                current = word;

                continue;
            }

            current = std::move(candidate);
        }

        wrapped.push_back(std::move(current));
    }

    return wrapped;
}

void gui_ml_text_ctrl::draw() const {
    const auto profile = get_profile();
    if (!profile) {
        return;
    }

    const auto bounds = get_bounds();
    const auto height = line_height();
    const auto wrapped = wrap();

    auto y = bounds.y;
    for (auto i = scroll_; i < wrapped.size() && y + height <= bounds.y + bounds.height; ++i) {
        profile->draw_text(wrapped[i], {bounds.x, y, bounds.width, height}, alignment::left);
        y += height;
    }

    draw_children();
}
