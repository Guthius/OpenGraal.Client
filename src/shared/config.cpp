#include <shared/config.hpp>

#include <shared/text.hpp>

#include <cctype>
#include <format>
#include <fstream>

using namespace std;

namespace og::shared {
    namespace {
        auto is_comment(const string_view line) -> bool {
            return line.empty() || line.starts_with('#');
        }
    }

    auto settings::contains(const string_view key) const -> bool {
        return values_.contains(to_lower(key));
    }

    auto settings::get(const string_view key, const string_view fallback) const -> string {
        const auto match = values_.find(to_lower(key));
        if (match == values_.end() || match->second.empty()) {
            return string(fallback);
        }

        return match->second.front();
    }

    auto settings::get_int(const string_view key, const int fallback) const -> int {
        const auto value = get(key);

        return value.empty() ? fallback : to_int(value, fallback);
    }

    auto settings::get_bool(const string_view key, const bool fallback) const -> bool {
        const auto value = get(key);

        return value.empty() ? fallback : og::shared::to_bool(value, fallback);
    }

    auto settings::get_list(const string_view key) const -> vector<string> {
        const auto match = values_.find(to_lower(key));
        if (match == values_.end()) {
            return {};
        }

        vector<string> items;
        for (const auto &value : match->second) {
            for (const auto &item : split(value, ',')) {
                if (auto trimmed = trim(item); !trimmed.empty()) {
                    items.push_back(std::move(trimmed));
                }
            }
        }

        return items;
    }

    void settings::add(string key, string value) {
        values_[to_lower(key)].push_back(std::move(value));
    }

    auto load_settings(istream &is) -> settings {
        settings result;
        string line;

        while (getline(is, line)) {
            const auto trimmed = trim(line);
            if (is_comment(trimmed)) {
                continue;
            }

            const auto separator = trimmed.find('=');
            if (separator == string::npos) {
                continue;
            }

            result.add(trim(trimmed.substr(0, separator)), trim(trimmed.substr(separator + 1)));
        }

        return result;
    }

    auto load_settings(const filesystem::path &path) -> result<settings> {
        ifstream ifs(path);
        if (!ifs) {
            return make_error(format("could not open '{}'", path.string()));
        }

        return load_settings(ifs);
    }

    auto parse_folder_category(const string_view name) -> optional<folder_category> {
        if (name == "file") return folder_category::file;
        if (name == "level") return folder_category::level;
        if (name == "head") return folder_category::head;
        if (name == "body") return folder_category::body;
        if (name == "sword") return folder_category::sword;
        if (name == "shield") return folder_category::shield;

        return nullopt;
    }

    auto load_folder_config(istream &is) -> vector<folder_rule> {
        vector<folder_rule> rules;
        string line;

        while (getline(is, line)) {
            const auto trimmed = trim(line);
            if (is_comment(trimmed)) {
                continue;
            }

            const auto tokens = split_whitespace(trimmed);
            if (tokens.size() < 2) {
                continue;
            }

            const auto category = parse_folder_category(to_lower(tokens[0]));
            if (!category) {
                continue;
            }

            rules.push_back(folder_rule{
                .category = *category,
                .pattern = tokens[1],
            });
        }

        return rules;
    }

    auto load_folder_config(const filesystem::path &path) -> result<vector<folder_rule>> {
        ifstream ifs(path);
        if (!ifs) {
            return make_error(format("could not open '{}'", path.string()));
        }

        return load_folder_config(ifs);
    }

    auto match_folder_pattern(const string_view pattern, const string_view path) -> bool {
        size_t p = 0;
        size_t s = 0;
        auto star = string_view::npos;
        size_t star_matched = 0;

        const auto lower = [](const char ch) {
            return static_cast<char>(tolower(static_cast<unsigned char>(ch)));
        };

        while (s < path.size()) {
            if (p < pattern.size() && pattern[p] == '*') {
                star = p++;
                star_matched = s;
            } else if (p < pattern.size() &&
                       ((pattern[p] == '?' && path[s] != '/') || lower(pattern[p]) == lower(path[s]))) {
                ++p;
                ++s;
            } else if (star != string_view::npos && path[star_matched] != '/') {
                p = star + 1;
                s = ++star_matched;
            } else {
                return false;
            }
        }

        while (p < pattern.size() && pattern[p] == '*') {
            ++p;
        }

        return p == pattern.size();
    }
}
