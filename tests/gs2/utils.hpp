#pragma once

#include <gs2/ast.hpp>
#include <gs2/eval.hpp>

template <typename T>
constexpr auto make_expr(T value) -> og::gs2::ast::expr {
    return std::make_unique<T>(std::move(value));
}

template <typename T>
auto has_value(const og::gs2::expected_value &result, const T &expected_value) -> bool {
    if (!result.has_value()) {
        return false;
    }

    if (auto *value = get_if<T>(&*result)) {
        return (*value) == expected_value;
    }

    return false;
}
