#include <gs2/parser.hpp>

namespace og::gs2 {
    using namespace std;

    namespace {
        template <typename T>
        constexpr auto make_node(T value) -> unique_ptr<T> {
            return make_unique<T>(std::move(value));
        }

        struct parser_impl {
            using expected_expr = expected<ast::expr, error>;
            using expected_expr_list = expected<ast::expr_list, error>;
            using expected_block_stmt = expected<ast::block_stmt, error>;
            using expected_stmt = expected<ast::stmt, error>;
            using expected_string = expected<string, error>;
            using expected_switch_stmt_item = expected<ast::switch_stmt::item, error>;
            using expected_token = expected<token, error>;

            tokens &tokens;
            size_t pos = 0;

            [[nodiscard]]
            auto make_error(string message) const -> error {
                return error{
                    .source = error_source::parser,
                    .kind = error_kind::syntax_error,
                    .message = std::move(message),
                    .position = peek().position,
                };
            };

            [[nodiscard]]
            auto peek() const -> const token & {
                static const token eof{
                    .kind = token_kind::eof,
                };
                return pos < tokens.size() ? tokens[pos] : eof;
            }

            [[nodiscard]]
            auto check(token_kind kind) const -> bool {
                return peek().kind == kind;
            }

            [[nodiscard]]
            auto check_keyword(keyword_kind kind) const -> bool {
                return peek().kind == token_kind::keyword && peek().keyword == kind;
            }

            [[nodiscard]]
            auto check_identifier(string_view lexeme) const -> bool {
                return peek().kind == token_kind::identifier && peek().lexeme == lexeme;
            }

            auto match(token_kind kind) -> bool {
                if (!check(kind)) {
                    return false;
                }
                ++pos;
                return true;
            }

            auto match_keyword(keyword_kind kind) -> bool {
                if (!check_keyword(kind)) {
                    return false;
                }
                ++pos;
                return true;
            }

            void skip_semicolons() {
                while (!at_end() && match(token_kind::semicolon)) {
                }
            }

            auto expect(token_kind kind, string_view message) -> expected_token {
                if (!check(kind)) {
                    const auto &token = peek();
                    return unexpected(error{
                        .source = error_source::parser,
                        .kind = error_kind::syntax_error,
                        .message = format("{} (got '{}')", message, token.lexeme),
                        .position = token.position,
                    });
                }
                return advance();
            }

            auto expect_keyword(keyword_kind kind, string_view message) -> expected_token {
                if (!check_keyword(kind)) {
                    const auto &token = peek();
                    return unexpected(error{
                        .source = error_source::parser,
                        .kind = error_kind::syntax_error,
                        .message = format("{} (got '{}')", message, token.lexeme),
                        .position = token.position,
                    });
                }
                return advance();
            }

            auto expect_identifier(string_view message) -> expected_token {
                return expect(token_kind::identifier, message);
            }

            [[nodiscard]]
            auto at_end() const -> bool {
                return check(token_kind::eof);
            }

            auto advance() -> const token & {
                const auto &tok = peek();
                if (!at_end()) {
                    ++pos;
                }
                return tok;
            };

            auto parse_expr() -> expected_expr {
                return parse_assign();
            }

            auto parse_assign() -> expected_expr {
                auto left = parse_ternary();
                if (!left) {
                    return left;
                }

                static constexpr array ops = {
                    token_kind::op_assign,
                    token_kind::op_assign_add,
                    token_kind::op_assign_subtract,
                    token_kind::op_assign_multiply,
                    token_kind::op_assign_divide,
                    token_kind::op_assign_modulo,
                    token_kind::op_assign_concat,
                    token_kind::op_assign_exponent,
                    token_kind::op_assign_bitand,
                    token_kind::op_assign_bitor,
                    token_kind::op_assign_shl,
                    token_kind::op_assign_shr,
                };

                for (auto op : ops) {
                    if (!check(op)) {
                        continue;
                    }

                    auto position = peek().position;
                    advance();

                    auto right = parse_assign();
                    if (!right) {
                        return right;
                    }

                    return make_node(ast::assign_expr{
                        .op = op,
                        .target = std::move(*left),
                        .value = std::move(*right),
                        .position = position,
                    });
                }

                return left;
            }

            auto parse_ternary() -> expected_expr {
                auto condition = parse_or();
                if (!condition) {
                    return condition;
                }

                if (!check(token_kind::question)) {
                    return condition;
                }

                auto position = peek().position;

                advance();

                auto then_branch = parse_expr();
                if (!then_branch) {
                    return then_branch;
                }

                if (auto match = expect(token_kind::colon, "expected ':' in ternary expression"); !match) {
                    return unexpected(match.error());
                }

                auto else_branch = parse_expr();
                if (!else_branch) {
                    return else_branch;
                }

                return make_node(ast::ternary_expr{
                    .condition = std::move(*condition),
                    .then_branch = std::move(*then_branch),
                    .else_branch = std::move(*else_branch),
                    .position = position,
                });
            }

            auto parse_or() -> expected_expr {
                auto left = parse_and();
                if (!left) {
                    return left;
                }

                while (check(token_kind::op_or)) {
                    auto position = peek().position;
                    auto op = peek().kind;

                    advance();

                    auto right = parse_and();
                    if (!right) {
                        return right;
                    }

                    left = make_node(ast::binary_expr{
                        .op = op,
                        .left = std::move(*left),
                        .right = std::move(*right),
                        .position = position,
                    });
                }

                return left;
            }

            auto parse_and() -> expected_expr {
                auto left = parse_bitwise_or();
                if (!left) {
                    return left;
                }

                while (check(token_kind::op_and)) {
                    auto position = peek().position;
                    auto op = peek().kind;

                    advance();

                    auto right = parse_bitwise_or();
                    if (!right) {
                        return right;
                    }

                    left = make_node(ast::binary_expr{
                        .op = op,
                        .left = std::move(*left),
                        .right = std::move(*right),
                        .position = position,
                    });
                }

                return left;
            }

            auto parse_bitwise_or() -> expected_expr {
                auto left = parse_bitwise_xor();
                if (!left) {
                    return left;
                }

                while (check(token_kind::pipe)) {
                    auto position = peek().position;
                    advance();

                    auto right = parse_bitwise_xor();
                    if (!right) {
                        return right;
                    }

                    left = make_node(ast::binary_expr{
                        .op = token_kind::pipe,
                        .left = std::move(*left),
                        .right = std::move(*right),
                        .position = position,
                    });
                }

                return left;
            }

            auto parse_bitwise_xor() -> expected_expr {
                auto left = parse_bitwise_and();
                if (!left) {
                    return left;
                }

                while (check_keyword(keyword_kind::xor_)) {
                    auto position = peek().position;
                    advance();

                    auto right = parse_bitwise_and();
                    if (!right) {
                        return right;
                    }

                    left = make_node(ast::binary_expr{
                        .op = token_kind::op_xor,
                        .left = std::move(*left),
                        .right = std::move(*right),
                        .position = position,
                    });
                }

                return left;
            }

            auto parse_bitwise_and() -> expected_expr {
                auto left = parse_in_range();
                if (!left) {
                    return left;
                }

                while (check(token_kind::op_bitand)) {
                    auto position = peek().position;
                    advance();

                    auto right = parse_in_range();
                    if (!right) {
                        return right;
                    }

                    left = make_node(ast::binary_expr{
                        .op = token_kind::op_bitand,
                        .left = std::move(*left),
                        .right = std::move(*right),
                        .position = position,
                    });
                }

                return left;
            }

            auto parse_in_range() -> expected_expr {
                auto left = parse_equality();
                if (!left) {
                    return left;
                }

                if (!check_keyword(keyword_kind::in)) {
                    return left;
                }

                auto position = peek().position;

                advance();

                if (check(token_kind::pipe) || check(token_kind::op_lt)) {
                    const auto min_inclusive = check(token_kind::pipe);
                    advance();

                    auto min = parse_concat();
                    if (!min) {
                        return min;
                    }

                    if (auto match = expect(token_kind::comma, "expected ',' in range"); !match) {
                        return unexpected(match.error());
                    }

                    auto max = parse_concat();
                    if (!max) {
                        return max;
                    }

                    const auto max_inclusive = check(token_kind::pipe);
                    if (!max_inclusive && !check(token_kind::op_gt)) {
                        return unexpected(make_error("expected '|' or '>' closing range"));
                    }

                    advance();

                    return make_node(ast::in_expr{
                        .value = std::move(*left),
                        .range = make_node(ast::range_expr{
                            .min = std::move(*min),
                            .max = std::move(*max),
                            .min_inclusive = min_inclusive,
                            .max_inclusive = max_inclusive,
                        }),
                        .position = position,
                    });
                }

                auto range = parse_postfix();
                if (!range) {
                    return range;
                }

                return make_node(ast::in_expr{
                    .value = std::move(*left),
                    .range = std::move(*range),
                    .position = position,
                });
            }

            auto parse_equality() -> expected_expr {
                auto left = parse_relational();
                if (!left) {
                    return left;
                }

                while (check(token_kind::op_eq) || check(token_kind::op_neq)) {
                    auto position = peek().position;
                    auto op = peek().kind;

                    advance();

                    auto right = parse_relational();
                    if (!right) {
                        return right;
                    }

                    left = make_node(ast::binary_expr{
                        .op = op,
                        .left = std::move(*left),
                        .right = std::move(*right),
                        .position = position,
                    });
                }

                return left;
            }

            auto parse_relational() -> expected_expr {
                auto left = parse_concat();
                if (!left) {
                    return left;
                }

                while (check(token_kind::op_gt) ||
                       check(token_kind::op_gte) ||
                       check(token_kind::op_lt) ||
                       check(token_kind::op_lte)) {
                    auto position = peek().position;
                    auto op = peek().kind;

                    advance();

                    auto right = parse_concat();
                    if (!right) {
                        return right;
                    }

                    left = make_node(ast::binary_expr{
                        .op = op,
                        .left = std::move(*left),
                        .right = std::move(*right),
                        .position = position,
                    });
                }

                return left;
            }

            auto parse_concat() -> expected_expr {
                auto left = parse_shift();
                if (!left) {
                    return left;
                }

                while (check(token_kind::op_concat) ||
                       check(token_kind::op_concat_spc) ||
                       check(token_kind::op_concat_nl)) {
                    auto position = peek().position;
                    auto op = peek().kind;

                    advance();

                    auto right = parse_shift();
                    if (!right) {
                        return right;
                    }

                    left = make_node(ast::binary_expr{
                        .op = op,
                        .left = std::move(*left),
                        .right = std::move(*right),
                        .position = position,
                    });
                }

                return left;
            }

            auto parse_shift() -> expected_expr {
                auto left = parse_additive();
                if (!left) {
                    return left;
                }

                while (check(token_kind::op_shl) || check(token_kind::op_shr)) {
                    auto position = peek().position;
                    auto op = peek().kind;
                    advance();

                    auto right = parse_additive();
                    if (!right) {
                        return right;
                    }

                    left = make_node(ast::binary_expr{
                        .op = op,
                        .left = std::move(*left),
                        .right = std::move(*right),
                        .position = position,
                    });
                }

                return left;
            }

            auto parse_additive() -> expected_expr {
                auto left = parse_multiplicative();
                if (!left) {
                    return left;
                }

                while (check(token_kind::op_add) ||
                       check(token_kind::op_subtract)) {
                    auto position = peek().position;
                    auto op = peek().kind;

                    advance();

                    auto right = parse_multiplicative();
                    if (!right) {
                        return right;
                    }

                    left = make_node(ast::binary_expr{
                        .op = op,
                        .left = std::move(*left),
                        .right = std::move(*right),
                        .position = position,
                    });
                }

                return left;
            }

            auto parse_multiplicative() -> expected_expr {
                auto left = parse_exponent();
                if (!left) {
                    return left;
                }

                while (check(token_kind::op_multiply) ||
                       check(token_kind::op_divide) ||
                       check(token_kind::op_modulo)) {
                    auto position = peek().position;
                    auto op = peek().kind;

                    advance();

                    auto right = parse_exponent();
                    if (!right) {
                        return right;
                    }

                    left = make_node(ast::binary_expr{
                        .op = op,
                        .left = std::move(*left),
                        .right = std::move(*right),
                        .position = position,
                    });
                }

                return left;
            }

            auto parse_exponent() -> expected_expr {
                auto left = parse_unary();
                if (!left) {
                    return left;
                }

                if (check(token_kind::op_exponent)) {
                    auto position = peek().position;
                    advance();

                    auto right = parse_exponent();
                    if (!right) {
                        return right;
                    }

                    return make_node(ast::binary_expr{
                        .op = token_kind::op_exponent,
                        .left = std::move(*left),
                        .right = std::move(*right),
                        .position = position,
                    });
                }

                return left;
            }

            auto parse_unary() -> expected_expr {
                if (check(token_kind::op_not) ||
                    check(token_kind::op_bitnot) ||
                    check(token_kind::op_subtract) ||
                    check(token_kind::op_concat) ||
                    check(token_kind::op_increment) ||
                    check(token_kind::op_decrement)) {
                    auto position = peek().position;
                    auto op = peek().kind;

                    advance();

                    auto operand = parse_unary();
                    if (!operand) {
                        return operand;
                    }

                    return make_node(ast::unary_expr{
                        .op = op,
                        .operand = std::move(*operand),
                        .prefix = true,
                        .position = position,
                    });
                }

                return parse_postfix();
            }

            auto parse_call_args() -> expected_expr_list {
                advance();

                auto args = parse_expr_list();
                if (!args) {
                    return unexpected(args.error());
                }

                if (auto match = expect(token_kind::rparen, "expected ')' after arguments"); !match) {
                    return unexpected(match.error());
                }

                return args;
            }

            auto parse_postfix() -> expected_expr {
                auto left = parse_primary();
                if (!left) {
                    return left;
                }

                if (const auto *created = get_if<std::unique_ptr<ast::new_expr>>(&*left)) {
                    if (!(*created)->body.empty()) {
                        return left;
                    }
                }

                while (true) {
                    auto position = peek().position;

                    if (check(token_kind::op_scope)) {
                        advance();

                        auto member = parse_primary();
                        if (!member) {
                            return unexpected(member.error());
                        }

                        left = make_node(ast::scope_resolution_expr{
                            .scope = std::move(*left),
                            .member = std::move(*member),
                            .position = position,
                        });
                    } else if (check(token_kind::dot)) {
                        advance();

                        auto member = parse_primary();
                        if (!member) {
                            return member;
                        }

                        left = make_node(ast::member_expr{
                            .object = std::move(*left),
                            .member = std::move(*member),
                            .position = position,
                        });
                    } else if (check(token_kind::lbracket)) {
                        advance();

                        auto index = parse_expr();
                        if (!index) {
                            return index;
                        }

                        if (auto match = expect(token_kind::rbracket, "expected ']'"); !match) {
                            return unexpected(match.error());
                        }

                        left = make_node(ast::index_expr{
                            .object = std::move(*left),
                            .index = std::move(*index),
                            .position = position,
                        });
                    } else if (check(token_kind::lparen)) {
                        auto args = parse_call_args();
                        if (!args) {
                            return unexpected(args.error());
                        }

                        left = make_node(ast::call_expr{
                            .callee = std::move(*left),
                            .args = std::move(*args),
                            .position = position,
                        });
                    } else if (check(token_kind::op_increment) || check(token_kind::op_decrement)) {
                        auto op = peek().kind;
                        advance();

                        left = make_node(ast::unary_expr{
                            .op = op,
                            .operand = std::move(*left),
                            .prefix = false,
                            .position = position,
                        });
                    } else {
                        break;
                    }
                }

                return left;
            }

            auto parse_primary() -> expected_expr {
                auto position = peek().position;

                if (check(token_kind::number)) {
                    auto tok = advance();
                    return make_node(ast::number_expr{
                        .value = tok.value,
                        .position = position,
                    });
                }

                if (check(token_kind::string)) {
                    auto tok = advance();
                    return make_node(ast::string_expr{
                        .value = tok.lexeme,
                        .position = position,
                    });
                }

                if (check_keyword(keyword_kind::null)) {
                    advance();
                    return make_node(ast::null_expr{
                        .position = position,
                    });
                }

                if (check_keyword(keyword_kind::new_)) {
                    return parse_new();
                }

                if (check(token_kind::lparen)) {
                    advance();

                    auto expr = parse_expr();
                    if (!expr) {
                        return expr;
                    }

                    if (auto match = expect(token_kind::rparen, "expected ')' after expression"); !match) {
                        return unexpected(match.error());
                    }

                    return expr;
                }

                if (check(token_kind::lbrace)) {
                    advance();

                    ast::expr_list elements;
                    while (!at_end() && !check(token_kind::rbrace)) {
                        auto el = parse_expr();
                        if (!el) {
                            return el;
                        }

                        elements.push_back(std::move(*el));

                        if (!match(token_kind::comma)) {
                            break;
                        }
                    }

                    if (auto match = expect(token_kind::rbrace, "expected '}' after array literal"); !match) {
                        return unexpected(match.error());
                    }

                    return make_node(ast::array_expr{
                        .elements = std::move(elements),
                        .position = position,
                    });
                }

                if (check_keyword(keyword_kind::true_)) {
                    advance();
                    return make_node(ast::boolean_expr{
                        .value = true,
                        .position = position,
                    });
                }

                if (check_keyword(keyword_kind::false_)) {
                    advance();
                    return make_node(ast::boolean_expr{
                        .value = false,
                        .position = position,
                    });
                }

                if (check(token_kind::identifier)) {
                    auto tok = advance();
                    return make_node(ast::identifier_expr{
                        .name = tok.lexeme,
                        .position = position,
                    });
                }

                return unexpected(make_error(format(
                    "unexpected token '{}' in expression", peek().lexeme)));
            }

            auto parse_new_body() -> expected<ast::stmt_list, error> {
                if (!check(token_kind::lbrace)) {
                    return ast::stmt_list{};
                }

                auto block = parse_block();
                if (!block) {
                    return unexpected(block.error());
                }

                return std::move(block->body);
            }

            auto parse_new() -> expected_expr {
                auto position = peek().position;
                advance();

                if (check(token_kind::lbracket)) {
                    auto identifier_position = peek().position;
                    advance();

                    auto size = parse_expr();
                    if (!size) {
                        return size;
                    }

                    if (auto match = expect(token_kind::rbracket, "expected ']'"); !match) {
                        return unexpected(match.error());
                    }

                    ast::expr_list args;
                    args.push_back(std::move(*size));

                    return make_node(ast::new_expr{
                        .object_name = make_node(ast::identifier_expr{
                            .name = "Array",
                            .position = identifier_position,
                        }),
                        .args = std::move(args),
                        .position = position,
                    });
                }

                auto identifier = parse_primary();
                if (!identifier) {
                    return unexpected(identifier.error());
                }

                if (!match(token_kind::lparen)) {
                    return make_node(ast::new_expr{
                        .object_name = std::move(*identifier),
                        .position = position,
                    });
                }

                ast::expr_list args;
                while (!at_end() && !check(token_kind::rparen)) {
                    auto expr = parse_expr();
                    if (!expr) {
                        return expr;
                    }

                    args.push_back(std::move(*expr));
                    if (check(token_kind::rparen)) {
                        break;
                    }
                }

                if (auto match = expect(token_kind::rparen, "expected ')'"); !match) {
                    return unexpected(match.error());
                }

                auto body = parse_new_body();
                if (!body) {
                    return unexpected(body.error());
                }

                return make_node(ast::new_expr{
                    .object_name = std::move(*identifier),
                    .args = std::move(args),
                    .body = std::move(*body),
                    .position = position,
                });
            }

            auto parse_expr_list() -> expected_expr_list {
                ast::expr_list args;

                while (!at_end() && !check(token_kind::rparen)) {
                    auto arg = parse_expr();
                    if (!arg) {
                        return unexpected(arg.error());
                    }

                    args.push_back(std::move(*arg));

                    if (!match(token_kind::comma)) {
                        break;
                    }
                }

                return args;
            }

            auto parse_block() -> expected_block_stmt {
                auto position = peek().position;

                if (auto match = expect(token_kind::lbrace, "expected '{'"); !match) {
                    return unexpected(match.error());
                }

                ast::stmt_list body;
                while (!at_end() && !check(token_kind::rbrace)) {
                    auto stmt = parse_stmt();
                    if (!stmt) {
                        return unexpected(stmt.error());
                    }

                    body.push_back(std::move(*stmt));
                }

                if (auto match = expect(token_kind::rbrace, "expected '}'"); !match) {
                    return unexpected(match.error());
                }

                return ast::block_stmt{
                    .body = std::move(body),
                    .position = position,
                };
            }

            auto parse_if() -> expected_stmt {
                auto position = peek().position;
                advance();

                if (auto match = expect(token_kind::lparen, "expected '(' after 'if'"); !match) {
                    return unexpected(match.error());
                }

                auto cond = parse_expr();
                if (!cond) {
                    return unexpected(cond.error());
                }

                if (auto match = expect(token_kind::rparen, "expected ')' after if condition"); !match) {
                    return unexpected(match.error());
                }

                auto then_branch = parse_stmt();
                if (!then_branch) {
                    return unexpected(then_branch.error());
                }

                optional<ast::stmt> else_branch;
                if (check_keyword(keyword_kind::else_)) {
                    advance();
                    auto else_stmt = parse_stmt();
                    if (!else_stmt) {
                        return unexpected(else_stmt.error());
                    }

                    else_branch = std::move(*else_stmt);
                }

                return make_node(ast::if_stmt{
                    .condition = std::move(*cond),
                    .then_branch = std::move(*then_branch),
                    .else_branch = std::move(else_branch),
                    .position = position,
                });
            }

            auto parse_while() -> expected_stmt {
                auto position = peek().position;
                advance();

                if (auto match = expect(token_kind::lparen, "expected '(' after 'while'"); !match) {
                    return unexpected(match.error());
                }

                auto cond = parse_expr();
                if (!cond) {
                    return unexpected(cond.error());
                }

                if (auto match = expect(token_kind::rparen, "expected ')' after while condition"); !match) {
                    return unexpected(match.error());
                }

                auto body = parse_stmt();
                if (!body) {
                    return unexpected(body.error());
                }

                return make_node(ast::while_stmt{
                    .condition = std::move(*cond),
                    .body = std::move(*body),
                    .position = position,
                });
            }

            auto parse_for() -> expected_stmt {
                auto position = peek().position;
                advance();

                if (auto match = expect(token_kind::lparen, "expected '(' after 'for'"); !match) {
                    return unexpected(match.error());
                }

                optional<ast::expr> initializer;
                if (!check(token_kind::semicolon)) {
                    auto result = parse_expr();
                    if (!result) {
                        return unexpected(result.error());
                    }
                    initializer = std::move(*result);
                }

                if (check(token_kind::colon)) {
                    advance();

                    if (!initializer) {
                        return unexpected(make_error("foreach requires a loop variable"));
                    }

                    auto iterable = parse_expr();
                    if (!iterable) {
                        return unexpected(iterable.error());
                    }

                    if (auto match = expect(token_kind::rparen, "expected ')'"); !match) {
                        return unexpected(match.error());
                    }

                    auto body = parse_stmt();
                    if (!body) {
                        return unexpected(body.error());
                    }

                    return make_node(ast::foreach_stmt{
                        .variable = std::move(*initializer),
                        .iterable = std::move(*iterable),
                        .body = std::move(*body),
                        .position = position,
                    });
                }

                if (auto match = expect(token_kind::semicolon, "expected ';' in for loop"); !match) {
                    return unexpected(match.error());
                }

                optional<ast::expr> condition;
                if (!check(token_kind::semicolon)) {
                    auto result = parse_expr();
                    if (!result) {
                        return unexpected(result.error());
                    }
                    condition = std::move(*result);
                }

                if (auto match = expect(token_kind::semicolon, "expected ';' in for loop"); !match) {
                    return unexpected(match.error());
                }

                optional<ast::expr> post;
                if (!check(token_kind::rparen)) {
                    auto result = parse_expr();
                    if (!result) {
                        return unexpected(result.error());
                    }
                    post = std::move(*result);
                }

                if (auto match = expect(token_kind::rparen, "expected ')' after while condition"); !match) {
                    return unexpected(match.error());
                }

                auto body = parse_stmt();
                if (!body) {
                    return unexpected(body.error());
                }

                return make_node(ast::for_stmt{
                    .initializer = std::move(initializer),
                    .condition = std::move(condition),
                    .post = std::move(post),
                    .body = std::move(*body),
                    .position = position,
                });
            }

            auto parse_with() -> expected_stmt {
                auto position = peek().position;
                advance();

                if (auto match = expect(token_kind::lparen, "expected '(' after 'with'"); !match) {
                    return unexpected(match.error());
                }

                auto object = parse_expr();
                if (!object) {
                    return unexpected(object.error());
                }

                if (auto match = expect(token_kind::rparen, "expected ')' after with expression"); !match) {
                    return unexpected(match.error());
                }

                auto body = parse_stmt();
                if (!body) {
                    return unexpected(body.error());
                }

                return make_node(ast::with_stmt{
                    .object = std::move(*object),
                    .body = std::move(*body),
                    .position = position,
                });
            }

            auto parse_return() -> expected_stmt {
                auto position = peek().position;
                advance();

                optional<ast::expr> value;
                if (!at_end() && !check(token_kind::semicolon) && !check(token_kind::rbrace)) {
                    auto result = parse_expr();
                    if (!result) {
                        return unexpected(result.error());
                    }
                    value = std::move(*result);
                }

                skip_semicolons();

                return make_node(ast::return_stmt{
                    .value = std::move(value),
                    .position = position,
                });
            }

            auto parse_function_name() -> expected_string {
                auto ident = expect_identifier("expected function name");
                if (!ident) {
                    return unexpected(ident.error());
                }

                auto name = ident->lexeme;
                if (!match(token_kind::dot)) {
                    return std::move(name);
                }

                ident = expect_identifier("expected function name");
                if (!ident) {
                    return unexpected(ident.error());
                }

                name += '.';
                name += ident->lexeme;

                return std::move(name);
            }

            auto parse_function_param_name() -> expected_string {
                auto token = expect_identifier("expected parameter name");
                if (!token) {
                    return unexpected(token.error());
                }

                string name = token->lexeme;
                if (check(token_kind::dot)) {
                    advance();

                    auto rest = expect_identifier("expected identifier after '.' in parameter name");
                    if (!rest) {
                        return unexpected(rest.error());
                    }

                    name = rest->lexeme;
                }

                return name;
            }

            auto parse_function() -> expected_stmt {
                auto position = peek().position;

                auto is_public = match_keyword(keyword_kind::public_);
                advance();

                auto name = parse_function_name();
                if (!name) {
                    return unexpected(name.error());
                }

                if (auto match = expect(token_kind::lparen, "expected '(' after function name"); !match) {
                    return unexpected(match.error());
                }

                vector<string> params;
                while (!at_end() && !check(token_kind::rparen)) {
                    auto param_name = parse_function_param_name();
                    if (!param_name) {
                        return unexpected(param_name.error());
                    }

                    params.push_back(*param_name);

                    if (!match(token_kind::comma)) {
                        break;
                    }
                }

                if (auto match = expect(token_kind::rparen, "expected ')' after function parameters"); !match) {
                    return unexpected(match.error());
                }

                auto body = parse_stmt();
                if (!body) {
                    return unexpected(body.error());
                }

                return make_node(ast::function_stmt{
                    .name = std::move(*name),
                    .params = std::move(params),
                    .body = std::move(*body),
                    .is_public = is_public,
                    .position = position,
                });
            }

            auto parse_case() -> expected_switch_stmt_item {
                auto position = peek().position;

                advance();

                auto value = parse_expr();
                if (!value) {
                    return unexpected(value.error());
                }

                if (auto match = expect(token_kind::colon, "expected ':' after case"); !match) {
                    return unexpected(match.error());
                }

                return ast::switch_stmt::label{
                    .value = std::move(*value),
                    .position = position,
                };
            }

            auto parse_switch_item() -> expected_switch_stmt_item {
                if (check_keyword(keyword_kind::default_)) {
                    auto position = peek().position;

                    advance();

                    if (auto match = expect(token_kind::colon, "expected ':' after default"); !match) {
                        return unexpected(match.error());
                    }

                    return ast::switch_stmt::label{
                        .value = nullopt,
                        .position = position,
                    };
                }

                if (check_keyword(keyword_kind::case_)) {
                    return parse_case();
                }

                return parse_stmt();
            }

            auto parse_switch() -> expected_stmt {
                auto position = peek().position;
                advance();

                if (auto match = expect(token_kind::lparen, "expected '(' after 'switch'"); !match) {
                    return unexpected(match.error());
                }

                auto operand = parse_expr();
                if (!operand) {
                    return unexpected(operand.error());
                }

                if (auto match = expect(token_kind::rparen, "expected ')' after switch operand"); !match) {
                    return unexpected(match.error());
                }

                if (auto match = expect(token_kind::lbrace, "expected '{'"); !match) {
                    return unexpected(match.error());
                }

                vector<ast::switch_stmt::item> items;
                while (!at_end() && !check(token_kind::rbrace)) {
                    auto item = parse_switch_item();
                    if (!item) {
                        return unexpected(item.error());
                    }

                    items.push_back(std::move(*item));

                    skip_semicolons();

                    if (check(token_kind::rbrace)) {
                        break;
                    }
                }

                if (auto match = expect(token_kind::rbrace, "expected '}'"); !match) {
                    return unexpected(match.error());
                }

                return make_node(ast::switch_stmt{
                    .operand = std::move(*operand),
                    .items = std::move(items),
                    .position = position,
                });
            }

            auto parse_enum() -> expected_stmt {
                auto position = peek().position;
                advance();

                optional<string> name;
                if (check(token_kind::identifier)) {
                    name = peek().lexeme;
                    advance();
                }

                if (auto match = expect(token_kind::lbrace, "expected '{' after enum"); !match) {
                    return unexpected(match.error());
                }

                vector<ast::enum_stmt::entry> entries;
                while (!at_end() && !check(token_kind::rbrace)) {
                    auto entry_position = peek().position;

                    auto ident = expect_identifier("expected enum entry name");
                    if (!ident) {
                        return unexpected(ident.error());
                    }

                    optional<ast::expr> value;
                    if (match(token_kind::op_assign)) {
                        auto expr = parse_expr();
                        if (!expr) {
                            return unexpected(expr.error());
                        }

                        value = std::move(*expr);
                    }

                    entries.push_back(ast::enum_stmt::entry{
                        .name = ident->lexeme,
                        .value = std::move(value),
                        .position = entry_position,
                    });

                    if (!match(token_kind::comma)) {
                        break;
                    }
                }

                if (auto match = expect(token_kind::rbrace, "expected '}' after enum entries"); !match) {
                    return unexpected(match.error());
                }

                skip_semicolons();

                return make_node(ast::enum_stmt{
                    .name = std::move(name),
                    .entries = std::move(entries),
                    .position = position,
                });
            }

            auto parse_do_while() -> expected_stmt {
                auto position = peek().position;
                advance();

                auto body = parse_stmt();
                if (!body) {
                    return body;
                }

                if (auto match = expect_keyword(keyword_kind::while_, "expected while"); !match) {
                    return unexpected(match.error());
                }

                if (auto match = expect(token_kind::lparen, "expected '(' after while"); !match) {
                    return unexpected(match.error());
                }

                auto cond = parse_expr();
                if (!cond) {
                    return unexpected(cond.error());
                }

                if (auto match = expect(token_kind::rparen, "expected ')'"); !match) {
                    return unexpected(match.error());
                }

                skip_semicolons();

                return make_node(ast::do_while_stmt{
                    .body = std::move(*body),
                    .condition = std::move(*cond),
                    .position = position,
                });
            }

            auto parse_stmt() -> expected_stmt {
                auto position = peek().position;

                if (check(token_kind::lbrace)) {
                    auto block = parse_block();
                    if (!block) {
                        return unexpected(block.error());
                    }
                    return make_node(std::move(*block));
                }

                if (check_identifier("return")) {
                    return parse_return();
                }

                if (peek().kind == token_kind::keyword) {
                    switch (peek().keyword) {
                    case keyword_kind::if_:      return parse_if();
                    case keyword_kind::while_:   return parse_while();
                    case keyword_kind::for_:     return parse_for();
                    case keyword_kind::with:     return parse_with();
                    case keyword_kind::public_:
                    case keyword_kind::function: return parse_function();
                    case keyword_kind::switch_:  return parse_switch();
                    case keyword_kind::enum_:    return parse_enum();
                    case keyword_kind::do_:      return parse_do_while();

                    case keyword_kind::break_:
                        advance();
                        skip_semicolons();
                        return make_node(ast::break_stmt{
                            .position = position,
                        });
                        break;

                    case keyword_kind::continue_:
                        advance();
                        skip_semicolons();
                        return make_node(ast::continue_stmt{
                            .position = position,
                        });
                        break;

                    default: break;
                    }
                }

                auto expr = parse_expr();
                if (!expr) {
                    return unexpected(expr.error());
                }

                if (check(token_kind::comma)) {
                    ast::stmt_list body;
                    body.push_back(make_node(ast::expr_stmt{.expression = std::move(*expr), .position = position}));

                    while (match(token_kind::comma)) {
                        auto next = parse_expr();
                        if (!next) {
                            return unexpected(next.error());
                        }

                        body.push_back(make_node(ast::expr_stmt{.expression = std::move(*next), .position = position}));
                    }

                    skip_semicolons();

                    return make_node(ast::block_stmt{
                        .body = std::move(body),
                        .position = position,
                    });
                }

                skip_semicolons();

                return make_node(ast::expr_stmt{
                    .expression = std::move(*expr),
                    .position = position,
                });
            }

            auto parse() -> expected_stmt {
                auto position = peek().position;

                ast::stmt_list body;

                while (!at_end()) {
                    auto stmt = parse_stmt();
                    if (!stmt) {
                        return unexpected(stmt.error());
                    }

                    body.push_back(std::move(*stmt));

                    skip_semicolons();
                }

                if (body.size() == 1) {
                    return std::move(body[0]);
                }

                return make_node(ast::block_stmt{
                    .body = std::move(body),
                    .position = position,
                });
            }
        };
    }

    auto parse(const tokens &toks) -> parse_result {
        tokens filtered;

        filtered.reserve(toks.size());

        for (const auto &tok : toks) {
            if (tok.kind != token_kind::comment) {
                filtered.push_back(tok);
            }
        }

        auto parser = parser_impl{
            .tokens = filtered,
        };

        auto result = parser.parse();
        if (!result) {
            return unexpected(result.error());
        }

        return std::move(*result);
    }
}
