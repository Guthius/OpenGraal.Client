#pragma once

#include "lexer.hpp"

#include <memory>
#include <variant>

namespace og::gs2::ast {
    struct number_expr;
    struct string_expr;
    struct boolean_expr;
    struct array_expr;
    struct null_expr;
    struct identifier_expr;
    struct member_expr;
    struct index_expr;
    struct call_expr;
    struct unary_expr;
    struct binary_expr;
    struct ternary_expr;
    struct assign_expr;
    struct new_expr;
    struct range_expr;
    struct in_expr;
    struct scope_resolution_expr;

    using expr = std::variant<
        std::unique_ptr<number_expr>,
        std::unique_ptr<string_expr>,
        std::unique_ptr<boolean_expr>,
        std::unique_ptr<array_expr>,
        std::unique_ptr<null_expr>,
        std::unique_ptr<identifier_expr>,
        std::unique_ptr<member_expr>,
        std::unique_ptr<index_expr>,
        std::unique_ptr<call_expr>,
        std::unique_ptr<unary_expr>,
        std::unique_ptr<binary_expr>,
        std::unique_ptr<ternary_expr>,
        std::unique_ptr<assign_expr>,
        std::unique_ptr<new_expr>,
        std::unique_ptr<range_expr>,
        std::unique_ptr<in_expr>,
        std::unique_ptr<scope_resolution_expr>>;

    using expr_list = std::vector<expr>;

    struct number_expr {
        double value;
        source_position position;
    };

    struct string_expr {
        std::string value;
        source_position position;
    };

    struct boolean_expr {
        bool value;
        source_position position;
    };

    struct array_expr {
        expr_list elements;
        source_position position;
    };

    struct null_expr {
        source_position position;
    };

    struct identifier_expr {
        std::string name;
        source_position position;
    };

    struct member_expr {
        expr object;
        expr member;
        source_position position;
    };

    struct index_expr {
        expr object;
        expr index;
        source_position position;
    };

    struct call_expr {
        expr callee;
        expr_list args;
        source_position position;
    };

    struct unary_expr {
        token_kind op;
        expr operand;
        bool prefix;
        source_position position;
    };

    struct binary_expr {
        token_kind op;
        expr left;
        expr right;
        source_position position;
    };

    struct ternary_expr {
        expr condition;
        expr then_branch;
        expr else_branch;
        source_position position;
    };

    struct assign_expr {
        token_kind op;
        expr target;
        expr value;
        source_position position;
    };

    struct range_expr {
        expr min;
        expr max;
        bool min_inclusive = true;
        bool max_inclusive = true;
        source_position position;
    };

    struct in_expr {
        expr value;
        expr range;
        source_position position;
    };

    struct scope_resolution_expr {
        expr scope;
        expr member;
        source_position position;
    };

    struct block_stmt;
    struct expr_stmt;
    struct if_stmt;
    struct while_stmt;
    struct for_stmt;
    struct foreach_stmt;
    struct with_stmt;
    struct return_stmt;
    struct break_stmt;
    struct continue_stmt;
    struct function_stmt;
    struct switch_stmt;
    struct enum_stmt;
    struct do_while_stmt;

    using stmt = std::variant<
        std::unique_ptr<block_stmt>,
        std::unique_ptr<expr_stmt>,
        std::unique_ptr<if_stmt>,
        std::unique_ptr<while_stmt>,
        std::unique_ptr<for_stmt>,
        std::unique_ptr<foreach_stmt>,
        std::unique_ptr<with_stmt>,
        std::unique_ptr<return_stmt>,
        std::unique_ptr<break_stmt>,
        std::unique_ptr<continue_stmt>,
        std::unique_ptr<function_stmt>,
        std::unique_ptr<switch_stmt>,
        std::unique_ptr<enum_stmt>,
        std::unique_ptr<do_while_stmt>>;

    using stmt_list = std::vector<stmt>;

    struct new_expr {
        expr object_name;
        expr_list args;
        stmt_list body;
        source_position position;
    };

    struct block_stmt {
        stmt_list body;
        source_position position;
    };

    struct expr_stmt {
        expr expression;
        source_position position;
    };

    struct if_stmt {
        expr condition;
        stmt then_branch;
        std::optional<stmt> else_branch;
        source_position position;
    };

    struct while_stmt {
        expr condition;
        stmt body;
        source_position position;
    };

    struct for_stmt {
        std::optional<expr> initializer;
        std::optional<expr> condition;
        std::optional<expr> post;
        stmt body;
        source_position position;
    };

    struct foreach_stmt {
        expr variable;
        expr iterable;
        stmt body;
        source_position position;
    };

    struct with_stmt {
        expr object;
        stmt body;
        source_position position;
    };

    struct return_stmt {
        std::optional<expr> value;
        source_position position;
    };

    struct break_stmt {
        source_position position;
    };

    struct continue_stmt {
        source_position position;
    };

    struct function_stmt {
        std::string name;
        std::vector<std::string> params;
        stmt body;
        bool is_public;
        source_position position;
    };

    struct switch_stmt {
        struct label {
            std::optional<expr> value;
            source_position position;
        };

        using item = std::variant<label, stmt>;

        expr operand;
        std::vector<item> items;
        source_position position;
    };

    struct enum_stmt {
        struct entry {
            std::string name;
            std::optional<expr> value;
            source_position position;
        };

        std::optional<std::string> name;
        std::vector<entry> entries;
        source_position position;
    };

    struct do_while_stmt {
        stmt body;
        expr condition;
        source_position position;
    };

    void print(std::ostream &os, const stmt &stmt);
    auto to_string(const ast::expr &expr) -> std::string;

    auto operator<<(std::ostream &os, const ast::expr &expr) -> std::ostream &;
}
