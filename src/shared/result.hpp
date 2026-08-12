#pragma once

#include <expected>
#include <string>

namespace og::shared {
    template <typename T>
    using result = std::expected<T, std::string>;

    [[nodiscard]] auto make_error(std::string message) -> std::unexpected<std::string>;
}
