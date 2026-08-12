#pragma once

#include "ast.hpp"
#include "context.hpp"

namespace og::gs2 {
    auto eval(context &context, const ast::expr &expr) -> expected_value;
    auto eval(context &context, const ast::stmt &stmt) -> expected_void;
    auto eval(context &context, const ast::function_stmt &fn, const values &args) -> expected_value;
}
