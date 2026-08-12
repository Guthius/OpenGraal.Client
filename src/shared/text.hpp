#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace og::shared {
    [[nodiscard]] auto to_lower(std::string_view str) -> std::string;
    [[nodiscard]] auto trim(std::string_view str) -> std::string;
    [[nodiscard]] auto iequals(std::string_view left, std::string_view right) -> bool;

    [[nodiscard]] auto split(std::string_view str, char separator) -> std::vector<std::string>;
    [[nodiscard]] auto split_whitespace(std::string_view str) -> std::vector<std::string>;

    [[nodiscard]] auto to_int(std::string_view str, int fallback = 0) -> int;
    [[nodiscard]] auto to_float(std::string_view str, float fallback = 0.0f) -> float;
    [[nodiscard]] auto to_bool(std::string_view str, bool fallback = false) -> bool;
}
