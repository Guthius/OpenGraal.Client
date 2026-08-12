#include "level_sign.hpp"

#include <utility>

namespace {
    constexpr int sign_width = 32;
    constexpr int sign_height = 16;
}

level_sign::level_sign(const float x, const float y, std::string text)
    : rectangle_{x, y, sign_width, sign_height},
      text_(std::move(text)) {
}
