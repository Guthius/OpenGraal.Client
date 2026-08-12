#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>

namespace og::gs2 {
    struct registry_hash {
        using is_transparent = void;

        auto operator()(std::string_view sv) const {
            return std::hash<std::string_view>{}(sv);
        }

        auto operator()(const std::string &str) const {
            return std::hash<std::string>{}(str);
        }
    };

    template <typename TValue>
    using registry = std::unordered_map<std::string, TValue, registry_hash, std::equal_to<>>;

    constexpr auto fold_case(const char c) -> char {
        return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
    }

    inline auto fold_case(const std::string_view text) -> std::string {
        std::string folded(text);
        std::ranges::transform(folded, folded.begin(), [](const char c) { return fold_case(c); });

        return folded;
    }

    struct folded_hash {
        using is_transparent = void;

        auto operator()(const std::string_view sv) const -> size_t {
            size_t hash = 14695981039346656037ULL;
            for (const auto c : sv) {
                hash = (hash ^ static_cast<unsigned char>(fold_case(c))) * 1099511628211ULL;
            }

            return hash;
        }
    };

    struct folded_equal {
        using is_transparent = void;

        auto operator()(const std::string_view lhs, const std::string_view rhs) const -> bool {
            return std::ranges::equal(lhs, rhs, [](const char a, const char b) { return fold_case(a) == fold_case(b); });
        }
    };

    template <typename TValue>
    using folded_registry = std::unordered_map<std::string, TValue, folded_hash, folded_equal>;
}
