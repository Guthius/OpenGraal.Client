#include "carry_object.hpp"

auto get_carry_object_rect(const carry_object_type type) -> Rectangle {
    switch (type) {
    case carry_object_type::bush:        return {0.0f, 338.0f, 32, 32};
    case carry_object_type::sign:        return {32.0f, 338.0f, 32, 32};
    case carry_object_type::vase:        return {64.0f, 338.0f, 32, 32};
    case carry_object_type::stone:       return {96.0f, 338.0f, 32, 32};
    case carry_object_type::black_stone: return {96.0f, 370.0f, 32, 32};
    default:                             return {};
    }
}

auto get_carry_object_leap_type(const carry_object_type type) -> leap_effect_type {
    switch (type) {
    case carry_object_type::bush: return leap_effect_type::bush;
    case carry_object_type::vase: return leap_effect_type::stone;
    case carry_object_type::sign: return leap_effect_type::sign;
    default:                      return leap_effect_type::none;
    }
}
