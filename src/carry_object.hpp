#pragma once

#include <raylib.h>

#include "leap_effect.hpp"

enum class carry_object_type
{
	none,
	bush,
	sign,
	vase,
	stone,
	black_stone
};

auto get_carry_object_rect(carry_object_type type) -> Rectangle;
auto get_carry_object_leap_type(carry_object_type type) -> leap_effect_type;
