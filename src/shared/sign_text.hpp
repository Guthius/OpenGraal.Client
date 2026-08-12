#pragma once

#include <string>
#include <string_view>

namespace og::shared {
    [[nodiscard]] auto decode_sign_text(std::string_view str) -> std::string;
    [[nodiscard]] auto encode_sign_text(std::string_view str) -> std::string;
}
