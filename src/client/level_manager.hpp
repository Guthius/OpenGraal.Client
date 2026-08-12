#pragma once

#include "level.hpp"

#include <string>

auto load_level(const std::string &name) -> std::shared_ptr<level>;
void install_level(const std::string &name, const std::shared_ptr<level> &value);

[[nodiscard]]
auto has_level(const std::string &name) -> bool;
