#pragma once

#include <string>

#include "level.hpp"

auto load_level(const std::string &name) -> std::shared_ptr<level>;
