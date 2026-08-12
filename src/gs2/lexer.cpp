#include <gs2/lexer.hpp>

#include <array>
#include <cstdlib>
#include <format>
#include <iostream>
#include <print>
#include <sstream>

namespace og::gs2 {
    using namespace std;

    namespace {
        constexpr auto is_whitespace(char ch) -> bool {
            return ch == '\t' || ch == '\n' ||
                   ch == '\v' || ch == '\f' ||
                   ch == '\r' || ch == ' ';
        }

        constexpr auto is_alpha(char ch) -> bool {
            return (ch >= 'A' && ch <= 'Z') ||
                   (ch >= 'a' && ch <= 'z');
        }

        constexpr auto is_digit(char ch) -> bool {
            return ch >= '0' && ch <= '9';
        }

        constexpr auto is_hexdigit(char ch) -> bool {
            return (ch >= '0' && ch <= '9') ||
                   (ch >= 'a' && ch <= 'f') ||
                   (ch >= 'A' && ch <= 'F');
        }

        constexpr auto is_alphanumeric(char ch) -> bool {
            return is_alpha(ch) || is_digit(ch);
        }

        struct keyword_info {
            string_view keyword;
            keyword_kind kind;
        };

        constexpr auto make_keyword(string_view value, keyword_kind kind) -> keyword_info {
            return (keyword_info){
                .keyword = value,
                .kind = kind,
            };
        }

        constexpr array keywords = {
            make_keyword("if", keyword_kind::if_),
            make_keyword("else", keyword_kind::else_),
            make_keyword("while", keyword_kind::while_),
            make_keyword("for", keyword_kind::for_),
            make_keyword("function", keyword_kind::function),
            make_keyword("new", keyword_kind::new_),
            make_keyword("with", keyword_kind::with),
            make_keyword("in", keyword_kind::in),
            make_keyword("break", keyword_kind::break_),
            make_keyword("continue", keyword_kind::continue_),
            make_keyword("null", keyword_kind::null),
            make_keyword("NULL", keyword_kind::null),
            make_keyword("true", keyword_kind::true_),
            make_keyword("false", keyword_kind::false_),
            make_keyword("switch", keyword_kind::switch_),
            make_keyword("case", keyword_kind::case_),
            make_keyword("default", keyword_kind::default_),
            make_keyword("public", keyword_kind::public_),
            make_keyword("enum", keyword_kind::enum_),
            make_keyword("do", keyword_kind::do_),
            make_keyword("xor", keyword_kind::xor_),
        };

        auto get_keyword_kind(string_view lexeme) -> optional<keyword_kind> {
            for (const auto &info : keywords) {
                if (info.keyword == lexeme) {
                    return info.kind;
                }
            }

            return nullopt;
        }

        struct lexer {
            source_position current_position = {
                .line = 1,
                .column = 1,
            };

            istream &stream;

            explicit lexer(istream &stream) : stream(stream) {
            }

            auto eof() -> bool { return stream.eof(); }

            auto peek() -> char {
                return eof() ? 0 : static_cast<char>(stream.peek());
            }

            auto advance() -> char {
                if (eof()) {
                    return 0;
                }

                char ch;
                if (!stream.get(ch)) {
                    return 0;
                }

                current_position.column++;
                if (ch == '\n') {
                    current_position.column = 1;
                    current_position.line++;
                }

                return ch;
            }

            void skip_whitespace() {
                while (!eof() && is_whitespace(peek())) {
                    advance();
                }
            }

            auto read_identifier(source_position position) -> token {
                ostringstream oss;

                const auto prefixed = peek() == '$';
                if (prefixed) {
                    oss << advance();
                }

                while (!eof() && (peek() == '_' || is_alphanumeric(peek()))) {
                    oss << advance();
                }

                auto lexeme = oss.str();

                if (prefixed && lexeme.size() == 1) {
                    return (token){
                        .kind = token_kind::invalid,
                        .lexeme = std::move(lexeme),
                        .position = position,
                    };
                }

                if (lexeme == "SPC") {
                    return (token){
                        .kind = token_kind::op_concat_spc,
                        .lexeme = std::move(lexeme),
                        .position = position,
                    };
                }

                if (lexeme == "NL") {
                    return (token){
                        .kind = token_kind::op_concat_nl,
                        .lexeme = std::move(lexeme),
                        .position = position,
                    };
                }

                if (auto keyword = get_keyword_kind(lexeme); keyword) {
                    return (token){
                        .kind = token_kind::keyword,
                        .lexeme = std::move(lexeme),
                        .keyword = keyword.value(),
                        .position = position,
                    };
                }

                return (token){
                    .kind = token_kind::identifier,
                    .lexeme = std::move(lexeme),
                    .position = position,
                };
            }

            static auto string_to_double(string_view str) -> double {
                const auto *ptr = str.data();
                return strtod(ptr, nullptr);
            }

            static auto hex_string_to_double(string_view str) -> double {
                static constexpr auto base10 = 10;

                str = str.substr(2);
                if (str.empty()) {
                    return 0;
                }

                const auto *ptr = str.data();
                auto value = strtoull(ptr, nullptr, base10);

                return static_cast<double>(value);
            }

            auto read_fraction(source_position position) -> token {
                ostringstream oss;

                oss << '.';
                oss << advance();

                while (!eof() && is_digit(peek())) {
                    oss << advance();
                }

                auto lexeme = oss.str();

                return (token){
                    .kind = token_kind::number,
                    .lexeme = lexeme,
                    .position = position,
                    .value = string_to_double(lexeme),
                };
            }

            auto read_number(source_position position) -> token {
                ostringstream oss;

                oss << advance();

                if (peek() == 'x') {
                    oss << advance();
                    while (!eof() && is_hexdigit(peek())) {
                        oss << advance();
                    }

                    auto lexeme = oss.str();

                    return (token){
                        .kind = token_kind::number,
                        .lexeme = lexeme,
                        .position = position,
                        .value = hex_string_to_double(lexeme),
                    };
                }

                while (!eof() && is_digit(peek())) {
                    oss << advance();
                }

                if (peek() == '.') {
                    oss << advance();
                    while (!eof() && is_digit(peek())) {
                        oss << advance();
                    }
                }

                auto lexeme = oss.str();

                return (token){
                    .kind = token_kind::number,
                    .lexeme = lexeme,
                    .position = position,
                    .value = string_to_double(lexeme),
                };
            }

            auto read_string(source_position position, const char quote) -> token {
                advance();

                ostringstream oss;
                while (!eof()) {
                    auto ch = peek();

                    if (ch == quote) {
                        break;
                    }

                    if (ch == '\\') {
                        advance();
                        if (eof()) {
                            goto bad_string;
                        }

                        ch = advance();
                        switch (ch) {
                        case 'n':  oss << '\n'; continue;
                        case 'r':  oss << '\r'; continue;
                        case 't':  oss << '\t'; continue;
                        case '"':  oss << '"'; continue;
                        case '\'': oss << '\''; continue;
                        case '\\': oss << '\\'; continue;
                        default:   goto bad_string;
                        }
                    }

                    oss << advance();
                }

                advance();

                return (token){
                    .kind = token_kind::string,
                    .lexeme = oss.str(),
                    .position = position,
                };

            bad_string:
                return (token){
                    .kind = token_kind::invalid,
                    .lexeme = oss.str(),
                    .position = current_position,
                };
            }

            auto read_single_line_comment(source_position position) -> token {
                ostringstream oss;

                oss << '/' << advance();
                while (!eof() && peek() != '\n') {
                    oss << advance();
                }

                return (token){
                    .kind = token_kind::comment,
                    .lexeme = oss.str(),
                    .position = position,
                };
            }

            auto read_multi_line_comment(source_position position) -> token {
                ostringstream oss;

                oss << '/' << advance();
                while (!eof()) {
                    auto ch = advance();

                    oss << ch;
                    if (ch == '*' && peek() == '/') {
                        oss << advance();
                        break;
                    }
                }

                return (token){
                    .kind = token_kind::comment,
                    .lexeme = oss.str(),
                    .position = position,
                };
            }

            auto read_symbol(source_position position) -> token {
                auto ch = advance();
                if (ch == '/') {
                    switch (peek()) {
                    case '/': return read_single_line_comment(position);
                    case '*': return read_multi_line_comment(position);
                    default:  break;
                    }
                }

                ostringstream oss;
                oss << ch;

                auto kind = token_kind::invalid;

                auto match_next = [&](char expected, token_kind match_kind) -> bool {
                    if (peek() != expected) {
                        return false;
                    }
                    oss << advance();
                    kind = match_kind;
                    return true;
                };

                auto check_for_double = [&](token_kind single_op, token_kind double_op) -> token_kind {
                    if (peek() != '=') {
                        return single_op;
                    }
                    oss << advance();
                    return double_op;
                };

                switch (ch) {
                case '.':
                    if (is_digit(peek())) {
                        return read_fraction(position);
                    }
                    kind = token_kind::dot;
                    break;

                case ',': kind = token_kind::comma; break;
                case '?': kind = token_kind::question; break;

                case ':':
                    if (match_next(':', token_kind::op_scope) ||
                        match_next('=', token_kind::op_assign)) {
                        break;
                    }
                    kind = token_kind::colon;
                    break;

                case ';': kind = token_kind::semicolon; break;
                case '{': kind = token_kind::lbrace; break;
                case '}': kind = token_kind::rbrace; break;
                case '(': kind = token_kind::lparen; break;
                case ')': kind = token_kind::rparen; break;
                case '[': kind = token_kind::lbracket; break;
                case ']': kind = token_kind::rbracket; break;

                case '+':
                    if (match_next('+', token_kind::op_increment) ||  // ++
                        match_next('=', token_kind::op_assign_add)) { // +=
                        break;
                    }
                    kind = token_kind::op_add;
                    break;

                case '-':
                    if (match_next('-', token_kind::op_decrement) ||       // --
                        match_next('=', token_kind::op_assign_subtract)) { // -=
                        break;
                    }
                    kind = token_kind::op_subtract;
                    break;

                case '*': kind = check_for_double(token_kind::op_multiply, token_kind::op_assign_multiply); break;
                case '/': kind = check_for_double(token_kind::op_divide, token_kind::op_assign_divide); break;
                case '%': kind = check_for_double(token_kind::op_modulo, token_kind::op_assign_modulo); break;
                case '^': kind = check_for_double(token_kind::op_exponent, token_kind::op_assign_exponent); break;
                case '~': kind = token_kind::op_bitnot; break;
                case '@': kind = check_for_double(token_kind::op_concat, token_kind::op_assign_concat); break;
                case '<':
                    if (peek() == '<') {
                        oss << advance();
                        kind = check_for_double(token_kind::op_shl, token_kind::op_assign_shl);
                        break;
                    }
                    kind = check_for_double(token_kind::op_lt, token_kind::op_lte);
                    break;

                case '>':
                    if (peek() == '>') {
                        oss << advance();
                        kind = check_for_double(token_kind::op_shr, token_kind::op_assign_shr);
                        break;
                    }
                    kind = check_for_double(token_kind::op_gt, token_kind::op_gte);
                    break;
                case '!': kind = check_for_double(token_kind::op_not, token_kind::op_neq); break;
                case '=':
                    if (match_next('<', token_kind::op_lte) ||
                        match_next('>', token_kind::op_gte)) {
                        break;
                    }
                    kind = check_for_double(token_kind::op_assign, token_kind::op_eq);
                    break;

                case '|':
                    if (match_next('|', token_kind::op_or) ||
                        match_next('=', token_kind::op_assign_bitor)) {
                        break;
                    }
                    kind = token_kind::pipe;
                    break;

                case '&':
                    if (match_next('&', token_kind::op_and) ||
                        match_next('=', token_kind::op_assign_bitand)) {
                        break;
                    }
                    kind = token_kind::op_bitand;
                    break;

                default: kind = token_kind::invalid;
                }

                return (token){
                    .kind = kind,
                    .lexeme = oss.str(),
                    .position = position,
                };
            }

            auto next_token() -> token {
                skip_whitespace();

                if (eof()) {
                    return (token){
                        .kind = token_kind::eof,
                        .position = current_position,
                    };
                }

                auto ch = peek();
                if (ch == '_' || ch == '$' || is_alpha(ch)) {
                    return read_identifier(current_position);
                }

                if (is_digit(ch)) {
                    return read_number(current_position);
                }

                switch (ch) {
                case '"':
                case '\'': return read_string(current_position, ch);
                default:   return read_symbol(current_position);
                }
            }
        };

        constexpr auto reserved_token_vector_size = 100;
    }

    auto token_kind_string(token_kind kind) -> string_view {
        switch (kind) {
        case token_kind::eof:                return "eof";
        case token_kind::keyword:            return "keyword";
        case token_kind::identifier:         return "identifier";
        case token_kind::number:             return "number";
        case token_kind::string:             return "string";
        case token_kind::comment:            return "comment";
        case token_kind::invalid:            return "invalid";
        case token_kind::dot:                return "dot";
        case token_kind::question:           return "question";
        case token_kind::colon:              return "colon";
        case token_kind::semicolon:          return "semicolon";
        case token_kind::lbrace:             return "lbrace";
        case token_kind::rbrace:             return "rbrace";
        case token_kind::lparen:             return "lparen";
        case token_kind::rparen:             return "rparen";
        case token_kind::lbracket:           return "lbracket";
        case token_kind::rbracket:           return "rbracket";
        case token_kind::op_add:             return "op_add";
        case token_kind::op_subtract:        return "op_subtract";
        case token_kind::op_multiply:        return "op_multiply";
        case token_kind::op_divide:          return "op_divide";
        case token_kind::op_modulo:          return "op_modulo";
        case token_kind::op_exponent:        return "op_exponent";
        case token_kind::op_concat:          return "op_concat";
        case token_kind::op_concat_spc:      return "op_concat_spc";
        case token_kind::op_concat_nl:       return "op_concat_nl";
        case token_kind::op_lt:              return "op_lt";
        case token_kind::op_lte:             return "op_lte";
        case token_kind::op_gt:              return "op_gt";
        case token_kind::op_gte:             return "op_gte";
        case token_kind::op_eq:              return "op_eq";
        case token_kind::op_neq:             return "op_neq";
        case token_kind::op_and:             return "op_and";
        case token_kind::op_or:              return "op_or";
        case token_kind::op_not:             return "op_not";
        case token_kind::op_assign:          return "op_assign";
        case token_kind::op_assign_add:      return "op_assign_add";
        case token_kind::op_assign_subtract: return "op_assign_substract";
        case token_kind::op_assign_multiply: return "op_assign_multiply";
        case token_kind::op_assign_divide:   return "op_assign_divide";
        case token_kind::op_assign_modulo:   return "op_assign_modulo";
        case token_kind::op_assign_concat:   return "op_assign_concat";
        case token_kind::op_increment:       return "op_increment";
        case token_kind::op_decrement:       return "op_decrement";
        case token_kind::op_scope:           return "op_scope";
        case token_kind::op_bitand:          return "op_bitand";
        case token_kind::op_bitnot:          return "op_bitnot";
        case token_kind::op_xor:             return "op_xor";
        case token_kind::op_shl:             return "op_shl";
        case token_kind::op_shr:             return "op_shr";
        case token_kind::op_assign_bitand:   return "op_assign_bitand";
        case token_kind::op_assign_bitor:    return "op_assign_bitor";
        case token_kind::op_assign_exponent: return "op_assign_exponent";
        case token_kind::op_assign_shl:      return "op_assign_shl";
        case token_kind::op_assign_shr:      return "op_assign_shr";
        default:                             return "unknown";
        }
    }

    auto is_keyword(const string_view lexeme) -> bool {
        return get_keyword_kind(lexeme).has_value();
    }

    auto tokenize(istream &is) -> tokenize_result {
        if (!is) {
            return unexpected((error){
                .source = error_source::lexer,
                .kind = error_kind::stream_error,
                .message = "unable to read from stream",
            });
        }

        tokens result;

        result.reserve(reserved_token_vector_size);

        lexer lexer(is);
        while (true) {
            auto token = lexer.next_token();
            if (token.kind == token_kind::eof) {
                break;
            }

            if (token.kind == token_kind::invalid) {
                return unexpected((error){
                    .source = error_source::lexer,
                    .kind = error_kind::token_error,
                    .message = format(
                        "invalid token",
                        token.position.line,
                        token.position.column),
                    .position = token.position,
                });
            }

            result.push_back(token);
        }

        return result;
    }

    void print_tokens(istream &is) {
        auto result = og::gs2::tokenize(is);
        if (result.has_value()) {
            println("{:>5} |{:>5} | {:<20}| {}", "Line", "Col", "Type", "Lexeme");
            println("------+------+---------------------+---------------------");

            for (auto &tok : *result) {
                println(
                    "{:>5} |{:>5} | {:<20}| {}",
                    tok.position.line, tok.position.column,
                    og::gs2::token_kind_string(tok.kind),
                    tok.lexeme);
            }
        } else {
            println(cerr, "Error: {}", result.error().message);
        }
    }
}
