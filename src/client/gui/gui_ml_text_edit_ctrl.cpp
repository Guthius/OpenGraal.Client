#include "gui_ml_text_edit_ctrl.hpp"

#include "../dev_input.hpp"
#include "../font_manager.hpp"
#include "gui_utils.hpp"

#include <gs2/lexer.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>

namespace {
    constexpr float text_padding = 4.0f;
    constexpr float blink_interval = 0.5f;
    constexpr float bar_thickness = 17.0f;

    const nine_patch background{0, 1, 2, 3, -1, 4, 5, 6, 7};

    auto column_at(const Font &font, const float size, const std::string &line, const float x) -> size_t {
        for (size_t i = 0; i < line.size(); ++i) {
            const auto width = MeasureTextEx(font, line.substr(0, i + 1).c_str(), size, 1.0f).x;
            if (width > x) {
                return i;
            }
        }

        return line.size();
    }

    auto leading_spaces(const std::string &line) -> std::string {
        const auto end = line.find_first_not_of(" \t");

        return line.substr(0, end == std::string::npos ? line.size() : end);
    }

    auto is_word_start(const char ch) -> bool {
        return std::isalpha(static_cast<unsigned char>(ch)) != 0 || ch == '_';
    }

    auto is_word(const char ch) -> bool {
        return is_word_start(ch) || std::isdigit(static_cast<unsigned char>(ch)) != 0;
    }

    auto token_end(const std::string &line, const size_t from) -> size_t {
        const auto ch = line[from];

        if (ch == '/' && from + 1 < line.size() && line[from + 1] == '/') {
            return line.size();
        }

        if (ch == '"' || ch == '\'') {
            for (auto i = from + 1; i < line.size(); ++i) {
                if (line[i] == '\\') {
                    ++i;

                    continue;
                }

                if (line[i] == ch) {
                    return i + 1;
                }
            }

            return line.size();
        }

        if (std::isdigit(static_cast<unsigned char>(ch)) != 0) {
            auto i = from;
            while (i < line.size() && (std::isdigit(static_cast<unsigned char>(line[i])) != 0 || line[i] == '.')) {
                ++i;
            }

            return i;
        }

        if (is_word_start(ch)) {
            auto i = from;
            while (i < line.size() && is_word(line[i])) {
                ++i;
            }

            return i;
        }

        return from + 1;
    }
}

gui_ml_text_edit_ctrl::gui_ml_text_edit_ctrl() {
    set_parse_tags(false);
    set_word_wrap(false);

    v_bar_ = std::make_shared<gui_scrollbar_ctrl>();
    v_bar_->set_vertical(true);
    v_bar_->set_visible(false);
    v_bar_->changed = [this](const float value) { scroll_ = static_cast<size_t>(std::lround(value)); };

    h_bar_ = std::make_shared<gui_scrollbar_ctrl>();
    h_bar_->set_visible(false);
    h_bar_->changed = [this](const float value) { scroll_x_ = value; };
}

void gui_ml_text_edit_ctrl::set_scrollbar_profile(const std::shared_ptr<gui_control_profile> &profile) {
    v_bar_->set_profile(profile);
    h_bar_->set_profile(profile);
}

void gui_ml_text_edit_ctrl::set_text(const std::string &text) {
    gui_ml_text_ctrl::set_text(text);

    caret_line_ = 0;
    caret_column_ = 0;
    anchor_line_ = 0;
    anchor_column_ = 0;
    scroll_ = 0;
    scroll_x_ = 0.0f;
    selecting_ = false;
}

void gui_ml_text_edit_ctrl::set_lines(std::vector<std::string> lines) {
    gui_ml_text_ctrl::set_lines(std::move(lines));

    caret_line_ = 0;
    caret_column_ = 0;
    anchor_line_ = 0;
    anchor_column_ = 0;
    scroll_ = 0;
    scroll_x_ = 0.0f;
    selecting_ = false;
}

auto gui_ml_text_edit_ctrl::current_line() const -> const std::string & {
    static const std::string empty;

    return caret_line_ < lines_.size() ? lines_[caret_line_] : empty;
}

void gui_ml_text_edit_ctrl::set_caret(const size_t line, const size_t column) {
    if (lines_.empty()) {
        lines_.emplace_back();
    }

    caret_line_ = std::min(line, lines_.size() - 1);
    caret_column_ = std::min(column, lines_[caret_line_].size());
    blink_ = 0.0f;
}

auto gui_ml_text_edit_ctrl::text_area() const -> Rectangle {
    const auto bounds = get_bounds();

    return {
        0,
        0,
        bounds.width - (v_bar_ && v_bar_->is_visible() ? bar_thickness : 0.0f),
        bounds.height - (h_bar_ && h_bar_->is_visible() ? bar_thickness : 0.0f),
    };
}

auto gui_ml_text_edit_ctrl::content_width() const -> float {
    const auto profile = get_profile();
    if (!profile) {
        return 0.0f;
    }

    const auto size = profile->get_font_size();
    const auto font = load_font(profile->get_font(), size);
    if (!IsFontValid(font)) {
        return 0.0f;
    }

    auto widest = 0.0f;
    for (const auto &line : lines_) {
        widest = std::max(widest, MeasureTextEx(font, line.c_str(), size, 1.0f).x);
    }

    return widest + (text_padding * 2.0f);
}

auto gui_ml_text_edit_ctrl::position_at(const Vector2 pt) const -> caret_position {
    const auto height = line_height();
    if (lines_.empty() || height <= 0.0f) {
        return {0, 0};
    }

    const auto row = scroll_ + static_cast<size_t>(std::max(0.0f, pt.y - text_padding) / height);
    if (row >= lines_.size()) {
        return {lines_.size() - 1, lines_.back().size()};
    }

    const auto profile = get_profile();
    if (!profile) {
        return {row, 0};
    }

    const auto size = profile->get_font_size();
    const auto font = load_font(profile->get_font(), size);

    return {row, IsFontValid(font) ? column_at(font, size, lines_[row], pt.x - text_padding + scroll_x_) : 0};
}

auto gui_ml_text_edit_ctrl::has_selection() const -> bool {
    return anchor_line_ != caret_line_ || anchor_column_ != caret_column_;
}

auto gui_ml_text_edit_ctrl::selection_range() const -> std::pair<caret_position, caret_position> {
    const auto anchor = caret_position{anchor_line_, anchor_column_};
    const auto caret = caret_position{caret_line_, caret_column_};

    return anchor <= caret ? std::pair{anchor, caret} : std::pair{caret, anchor};
}

void gui_ml_text_edit_ctrl::clear_selection() {
    anchor_line_ = caret_line_;
    anchor_column_ = caret_column_;
}

void gui_ml_text_edit_ctrl::erase_selection() {
    if (!has_selection()) {
        return;
    }

    const auto [from, to] = selection_range();

    if (from.line == to.line) {
        lines_[from.line].erase(from.column, to.column - from.column);
    } else {
        lines_[from.line] = lines_[from.line].substr(0, from.column) + lines_[to.line].substr(to.column);
        lines_.erase(lines_.begin() + static_cast<long long>(from.line) + 1,
            lines_.begin() + static_cast<long long>(to.line) + 1);
    }

    set_caret(from.line, from.column);
    clear_selection();
}

void gui_ml_text_edit_ctrl::insert_text(const std::string &text) {
    erase_selection();

    if (lines_.empty()) {
        lines_.emplace_back();
    }

    lines_[caret_line_].insert(caret_column_, text);
    caret_column_ += text.size();
    clear_selection();
}

void gui_ml_text_edit_ctrl::insert_newline() {
    erase_selection();

    if (lines_.empty()) {
        lines_.emplace_back();
    }

    auto &line = lines_[caret_line_];
    auto tail = line.substr(caret_column_);
    const auto indent = auto_indenting_ ? leading_spaces(line) : std::string{};

    line.erase(caret_column_);
    lines_.insert(lines_.begin() + static_cast<long long>(caret_line_) + 1, indent + tail);

    ++caret_line_;
    caret_column_ = indent.size();
    clear_selection();
}

void gui_ml_text_edit_ctrl::erase_before_caret() {
    if (has_selection()) {
        erase_selection();

        return;
    }

    if (caret_column_ > 0) {
        lines_[caret_line_].erase(caret_column_ - 1, 1);
        --caret_column_;
        clear_selection();

        return;
    }

    if (caret_line_ == 0) {
        return;
    }

    const auto tail = lines_[caret_line_];

    lines_.erase(lines_.begin() + static_cast<long long>(caret_line_));
    --caret_line_;
    caret_column_ = lines_[caret_line_].size();
    lines_[caret_line_] += tail;
    clear_selection();
}

void gui_ml_text_edit_ctrl::erase_at_caret() {
    if (has_selection()) {
        erase_selection();

        return;
    }

    if (caret_column_ < lines_[caret_line_].size()) {
        lines_[caret_line_].erase(caret_column_, 1);

        return;
    }

    if (caret_line_ + 1 >= lines_.size()) {
        return;
    }

    lines_[caret_line_] += lines_[caret_line_ + 1];
    lines_.erase(lines_.begin() + static_cast<long long>(caret_line_) + 1);
}

void gui_ml_text_edit_ctrl::move_caret(const int lines, const int columns) {
    if (lines_.empty()) {
        return;
    }

    const auto line = std::clamp(static_cast<int>(caret_line_) + lines, 0, static_cast<int>(lines_.size()) - 1);
    auto column = static_cast<int>(caret_column_) + columns;

    if (columns < 0 && column < 0 && caret_line_ > 0) {
        caret_line_ = caret_line_ - 1;
        caret_column_ = lines_[caret_line_].size();

        return;
    }

    if (columns > 0 && column > static_cast<int>(lines_[caret_line_].size()) && caret_line_ + 1 < lines_.size()) {
        ++caret_line_;
        caret_column_ = 0;

        return;
    }

    caret_line_ = static_cast<size_t>(line);
    column = std::clamp(column, 0, static_cast<int>(lines_[caret_line_].size()));
    caret_column_ = static_cast<size_t>(column);
}

void gui_ml_text_edit_ctrl::ensure_caret_visible() {
    const auto height = line_height();
    if (height <= 0.0f) {
        return;
    }

    const auto area = text_area();
    const auto visible = static_cast<size_t>(std::max(1.0f, (area.height - text_padding) / height));

    if (caret_line_ < scroll_) {
        scroll_ = caret_line_;
    } else if (caret_line_ >= scroll_ + visible) {
        scroll_ = caret_line_ - visible + 1;
    }

    const auto profile = get_profile();
    if (!profile) {
        return;
    }

    const auto size = profile->get_font_size();
    const auto font = load_font(profile->get_font(), size);
    if (!IsFontValid(font)) {
        return;
    }

    const auto caret_x = MeasureTextEx(font, current_line().substr(0, caret_column_).c_str(), size, 1.0f).x;
    const auto span = std::max(1.0f, area.width - (text_padding * 2.0f));

    if (caret_x < scroll_x_) {
        scroll_x_ = caret_x;
    } else if (caret_x > scroll_x_ + span) {
        scroll_x_ = caret_x - span;
    }
}

void gui_ml_text_edit_ctrl::handle_keyboard_input() {
    const auto arrows_clear = [this](const int lines, const int columns) {
        move_caret(lines, columns);
        clear_selection();
    };

    if (dev_input::key_pressed(KEY_LEFT)) {
        arrows_clear(0, -1);
    }

    if (dev_input::key_pressed(KEY_RIGHT)) {
        arrows_clear(0, 1);
    }

    if (dev_input::key_pressed(KEY_UP)) {
        arrows_clear(-1, 0);
    }

    if (dev_input::key_pressed(KEY_DOWN)) {
        arrows_clear(1, 0);
    }

    if (dev_input::key_pressed(KEY_HOME)) {
        caret_column_ = 0;
        clear_selection();
    }

    if (dev_input::key_pressed(KEY_END)) {
        caret_column_ = current_line().size();
        clear_selection();
    }

    if (dev_input::key_pressed(KEY_ENTER)) {
        insert_newline();
    }

    if (dev_input::key_pressed(KEY_BACKSPACE)) {
        erase_before_caret();
    }

    if (dev_input::key_pressed(KEY_DELETE)) {
        erase_at_caret();
    }

    if (tab_spaces_ > 0 && dev_input::key_pressed(KEY_TAB)) {
        insert_text(std::string(static_cast<size_t>(tab_spaces_), ' '));
    }

    for (auto key = dev_input::char_pressed(); key > 0; key = dev_input::char_pressed()) {
        if (key >= 32 && key != 127) {
            insert_text(std::string(1, static_cast<char>(key)));
        }
    }
}

void gui_ml_text_edit_ctrl::layout_scrollbars() {
    const auto bounds = get_bounds();
    const auto height = line_height();
    if (height <= 0.0f) {
        return;
    }

    const auto total_height = static_cast<float>(lines_.size()) * height;
    const auto total_width = content_width();

    auto need_v = total_height > bounds.height;
    const auto need_h = total_width > bounds.width - (need_v ? bar_thickness : 0.0f);
    need_v = total_height > bounds.height - (need_h ? bar_thickness : 0.0f);

    v_bar_->set_visible(need_v);
    h_bar_->set_visible(need_h);

    const auto area = text_area();

    v_bar_->set_position({bounds.width - bar_thickness, 0});
    v_bar_->set_size({bar_thickness, area.height});
    v_bar_->set_range(static_cast<float>(lines_.size()), std::max(1.0f, area.height / height));
    v_bar_->set_value(static_cast<float>(scroll_));

    h_bar_->set_position({0, bounds.height - bar_thickness});
    h_bar_->set_size({area.width, bar_thickness});
    h_bar_->set_range(total_width, area.width);
    h_bar_->set_value(scroll_x_);

    scroll_ = static_cast<size_t>(std::lround(v_bar_->get_value()));
    scroll_x_ = h_bar_->get_value();
}

void gui_ml_text_edit_ctrl::update(const float dt) {
    if (!v_bar_->get_parent()) {
        add_child(v_bar_);
        add_child(h_bar_);
    }

    gui_ml_text_ctrl::update(dt);

    blink_ += dt;

    if (lines_.empty()) {
        lines_.emplace_back();
    }

    caret_line_ = std::min(caret_line_, lines_.size() - 1);
    caret_column_ = std::min(caret_column_, lines_[caret_line_].size());
    anchor_line_ = std::min(anchor_line_, lines_.size() - 1);
    anchor_column_ = std::min(anchor_column_, lines_[anchor_line_].size());

    if (has_focus() && !is_disabled()) {
        const auto before_line = caret_line_;
        const auto before_column = caret_column_;

        handle_keyboard_input();

        if (caret_line_ != before_line || caret_column_ != before_column) {
            ensure_caret_visible();
        }
    }

    if (is_mouse_over() && !selecting_) {
        if (const auto wheel = dev_input::mouse_wheel(); wheel != 0.0f) {
            const auto limit = lines_.size() - std::min(lines_.size(),
                                                   static_cast<size_t>(std::max(1.0f, text_area().height / std::max(1.0f, line_height()))));

            scroll_ = static_cast<size_t>(std::clamp(
                static_cast<int>(scroll_) - (static_cast<int>(wheel) * 3), 0, static_cast<int>(limit)));
        }
    }

    layout_scrollbars();
}

void gui_ml_text_edit_ctrl::on_mouse_pressed(const int button, const Vector2 pt) {
    if (button != MOUSE_BUTTON_LEFT || !CheckCollisionPointRec(pt, text_area())) {
        return;
    }

    focus();

    const auto position = position_at(pt);

    set_caret(position.line, position.column);
    clear_selection();

    selecting_ = true;
    capture_mouse();
}

void gui_ml_text_edit_ctrl::on_mouse_move(const Vector2 pt) {
    if (!selecting_) {
        return;
    }

    const auto position = position_at(pt);

    caret_line_ = position.line;
    caret_column_ = position.column;
    blink_ = 0.0f;

    ensure_caret_visible();
}

void gui_ml_text_edit_ctrl::on_mouse_released(const int /*button*/, const Vector2 /*pt*/) {
    if (selecting_) {
        selecting_ = false;

        release_mouse();
    }
}

auto gui_ml_text_edit_ctrl::spans_of(const std::string &line) const -> std::vector<span> {
    const auto profile = get_profile();
    if (!profile) {
        return {};
    }

    if (!syntax_highlighting_) {
        return {
            {.text = line, .color = profile->font_color}
        };
    }

    std::vector<span> spans;

    for (size_t i = 0; i < line.size();) {
        const auto end = token_end(line, i);
        auto text = line.substr(i, end - i);

        const auto color = [&]() -> Color {
            if (text.starts_with("//")) {
                return profile->font_color_na;
            }

            if (text.front() == '"' || text.front() == '\'') {
                return profile->font_color_link;
            }

            if (std::isdigit(static_cast<unsigned char>(text.front())) != 0) {
                return profile->font_color_link_hl;
            }

            if (is_word_start(text.front())) {
                return og::gs2::is_keyword(text) ? profile->font_color_hl : profile->font_color_sel;
            }

            return profile->font_color;
        }();

        spans.push_back({.text = std::move(text), .color = color});
        i = end;
    }

    return spans;
}

void gui_ml_text_edit_ctrl::draw_line(const std::string &line, const Vector2 at) const {
    const auto profile = get_profile();
    if (!profile) {
        return;
    }

    const auto size = profile->get_font_size();
    const auto font = load_font(profile->get_font(), size);
    if (!IsFontValid(font)) {
        return;
    }

    std::string before;
    for (const auto &[text, color] : spans_of(line)) {
        DrawTextEx(font, text.c_str(),
            {std::round(at.x + MeasureTextEx(font, before.c_str(), size, 1.0f).x), std::round(at.y)},
            size, 1.0f, color);

        before += text;
    }
}

void gui_ml_text_edit_ctrl::draw() const {
    const auto profile = get_profile();
    if (!profile) {
        return;
    }

    const auto bounds = get_bounds();
    const auto height = line_height();

    profile->draw_frame(bounds, false, is_disabled());
    profile->draw_nine_patch(bounds, background);

    const auto area = text_area();
    const auto clip = local_to_global_coord({0, 0});

    BeginScissorMode(
        static_cast<int>(clip.x + 1), static_cast<int>(clip.y + 1),
        std::max(0, static_cast<int>(area.width) - 2), std::max(0, static_cast<int>(area.height) - 2));

    const auto size = profile->get_font_size();
    const auto font = load_font(profile->get_font(), size);
    const auto [from, to] = selection_range();
    const auto measure = [&](const std::string &text) {
        return IsFontValid(font) ? MeasureTextEx(font, text.c_str(), size, 1.0f).x : 0.0f;
    };

    const auto text_dy = std::round(std::max(0.0f, (height - size) / 2.0f));

    auto y = bounds.y + text_padding;
    for (auto i = scroll_; i < lines_.size() && y + height <= bounds.y + area.height; ++i) {
        const auto &line = lines_[i];
        const auto x = bounds.x + text_padding - scroll_x_;

        if (has_selection() && i >= from.line && i <= to.line) {
            const auto begin = i == from.line ? from.column : 0;
            const auto end = i == to.line ? to.column : line.size();
            const auto start_x = measure(line.substr(0, begin));
            const auto width = std::max(3.0f, measure(line.substr(0, end)) - start_x);

            DrawRectangleRec({x + start_x, y, width, height}, profile->fill_color_hl);
        }

        draw_line(line, {x, y + text_dy});
        y += height;
    }

    if (has_focus() && caret_line_ >= scroll_ && height > 0.0f &&
        std::fmod(blink_, blink_interval * 2.0f) < blink_interval) {
        const auto caret_y = bounds.y + text_padding + (static_cast<float>(caret_line_ - scroll_) * height);

        if (caret_y + height <= bounds.y + area.height) {
            const auto x = measure(current_line().substr(0, caret_column_));

            DrawRectangleRec({std::round(bounds.x + text_padding + x - scroll_x_), caret_y, 1.0f, height}, profile->caret_color);
        }
    }

    EndScissorMode();

    draw_children();
}
