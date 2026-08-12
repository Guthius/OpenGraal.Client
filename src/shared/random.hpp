#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace og::shared {
    [[nodiscard]] auto random_bytes(std::size_t count) -> std::vector<std::uint8_t>;
    [[nodiscard]] auto equals_constant_time(std::string_view left, std::string_view right) -> bool;
}
