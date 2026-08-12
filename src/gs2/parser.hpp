#pragma once

#include "ast.hpp"

namespace og::gs2 {
    using parse_result = std::expected<ast::stmt, error>;

    auto parse(const tokens &toks) -> parse_result;
}
