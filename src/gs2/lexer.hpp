#pragma once

#include "error.hpp"

#include <expected>
#include <istream>
#include <vector>

namespace og::gs2 {
    enum class token_kind : uint8_t {
        eof,
        keyword,
        identifier,
        number,
        string,
        comment,
        invalid,
        dot,                // .
        comma,              // ,
        question,           // ?
        colon,              // :
        semicolon,          // ;
        pipe,               // |
        lbrace,             // {
        rbrace,             // }
        lparen,             // (
        rparen,             // )
        lbracket,           // [
        rbracket,           // ]
        op_add,             // +
        op_subtract,        // -
        op_multiply,        // *
        op_divide,          // /
        op_modulo,          // %
        op_exponent,        // ^
        op_concat,          // @
        op_concat_spc,      // SPC
        op_concat_nl,       // NL
        op_lt,              // <
        op_lte,             // <=
        op_gt,              // >
        op_gte,             // >=
        op_eq,              // ==
        op_neq,             // !=
        op_and,             // &&
        op_or,              // ||
        op_not,             // !
        op_assign,          // =
        op_assign_add,      // +=
        op_assign_subtract, // -=
        op_assign_multiply, // *=
        op_assign_divide,   // /=
        op_assign_modulo,   // %=
        op_assign_concat,   // @=
        op_increment,       // ++
        op_decrement,       // --,
        op_scope,           // ::
        op_bitand,          // &
        op_bitnot,          // ~
        op_xor,             // xor
        op_shl,             // <<
        op_shr,             // >>
        op_assign_bitand,   // &=
        op_assign_bitor,    // |=
        op_assign_exponent, // ^=
        op_assign_shl,      // <<=
        op_assign_shr,      // >>=
    };

    auto token_kind_string(token_kind kind) -> std::string_view;

    enum class keyword_kind : uint8_t {
        unknown,
        if_,
        else_,
        while_,
        for_,
        function,
        new_,
        with,
        in,
        break_,
        continue_,
        null,
        true_,
        false_,
        switch_,
        case_,
        default_,
        public_,
        enum_,
        do_,
        xor_,
    };

    struct token {
        token_kind kind;
        std::string lexeme;
        keyword_kind keyword;
        source_position position;
        double value;
    };

    using tokens = std::vector<token>;
    using tokenize_result = std::expected<tokens, error>;

    auto tokenize(std::istream &is) -> tokenize_result;
    void print_tokens(std::istream &is);

    [[nodiscard]] auto is_keyword(std::string_view lexeme) -> bool;
}
