#pragma once

#include "level.hpp"

#include <string>

auto load_level(const std::string &name) -> std::shared_ptr<level>;
