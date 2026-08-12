#include <shared/text.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>

using namespace std;

namespace og::shared {
    namespace {
        constexpr auto whitespace = " \t\r\n\v\f";

        auto lower_char(const unsigned char ch) -> char {
            return static_cast<char>(tolower(ch));
        }
    }

    auto to_lower(const string_view str) -> string {
        string result(str);

        ranges::transform(result, result.begin(), [](const unsigned char ch) {
            return lower_char(ch);
        });

        return result;
    }

    auto trim(const string_view str) -> string {
        const auto first = str.find_first_not_of(whitespace);
        if (first == string_view::npos) {
            return {};
        }

        const auto last = str.find_last_not_of(whitespace);

        return string(str.substr(first, last - first + 1));
    }

    auto iequals(const string_view left, const string_view right) -> bool {
        return ranges::equal(left, right, [](const unsigned char a, const unsigned char b) {
            return lower_char(a) == lower_char(b);
        });
    }

    auto split(const string_view str, const char separator) -> vector<string> {
        vector<string> parts;

        size_t start = 0;
        while (start <= str.size()) {
            const auto end = str.find(separator, start);
            if (end == string_view::npos) {
                parts.emplace_back(str.substr(start));
                break;
            }

            parts.emplace_back(str.substr(start, end - start));
            start = end + 1;
        }

        return parts;
    }

    auto split_whitespace(const string_view str) -> vector<string> {
        vector<string> parts;

        size_t start = 0;
        while (start < str.size()) {
            start = str.find_first_not_of(whitespace, start);
            if (start == string_view::npos) {
                break;
            }

            const auto end = str.find_first_of(whitespace, start);
            if (end == string_view::npos) {
                parts.emplace_back(str.substr(start));
                break;
            }

            parts.emplace_back(str.substr(start, end - start));
            start = end;
        }

        return parts;
    }

    auto to_int(const string_view str, const int fallback) -> int {
        const auto text = trim(str);

        int value = 0;
        const auto *first = text.data();
        const auto *last = text.data() + text.size();

        if (from_chars(first, last, value).ec != errc{}) {
            return static_cast<int>(to_float(text, static_cast<float>(fallback)));
        }

        return value;
    }

    auto to_float(const string_view str, const float fallback) -> float {
        const auto text = trim(str);
        if (text.empty()) {
            return fallback;
        }

        try {
            return stof(text);
        } catch (const exception &) {
            return fallback;
        }
    }

    auto to_bool(const string_view str, const bool fallback) -> bool {
        const auto text = to_lower(trim(str));
        if (text.empty()) {
            return fallback;
        }

        return text == "true" || text == "1" || text == "yes" || text == "on";
    }
}
