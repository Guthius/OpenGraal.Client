#pragma once

#include "environment.hpp"

#include <string_view>

namespace og::gs2 {
    class context;

    [[nodiscard]]
    auto resolve_path(context &ctx, std::string_view path, bool create = false) -> value;

    [[nodiscard]]
    auto find_method(environment &env, const value &target, std::string_view name) -> callable_ptr;

    void register_standard_functions(environment &env);

    [[nodiscard]]
    auto format_string(std::string_view format, const values &args, size_t first_arg) -> std::string;
}
