#include <gs2/ast.hpp>

#include <sstream>

using namespace std;

namespace og::gs2::ast {
    namespace {
        template <class... Ts>
        struct overloaded : Ts... {
            using Ts::operator()...;
        };

        auto operator<<(ostream &os, const token_kind kind) -> ostream & {
            switch (kind) {
            case token_kind::op_add:             os << '+'; break;
            case token_kind::op_subtract:        os << '-'; break;
            case token_kind::op_multiply:        os << '*'; break;
            case token_kind::op_divide:          os << '/'; break;
            case token_kind::op_modulo:          os << '%'; break;
            case token_kind::op_exponent:        os << '^'; break;
            case token_kind::op_concat:          os << '@'; break;
            case token_kind::op_concat_spc:      os << "SPC"; break;
            case token_kind::op_concat_nl:       os << "NL"; break;
            case token_kind::op_lt:              os << '<'; break;
            case token_kind::op_lte:             os << "<="; break;
            case token_kind::op_gt:              os << '>'; break;
            case token_kind::op_gte:             os << ">="; break;
            case token_kind::op_eq:              os << "=="; break;
            case token_kind::op_neq:             os << "!="; break;
            case token_kind::op_and:             os << "&&"; break;
            case token_kind::op_or:              os << "||"; break;
            case token_kind::op_not:             os << '!'; break;
            case token_kind::op_assign:          os << '='; break;
            case token_kind::op_assign_add:      os << "+="; break;
            case token_kind::op_assign_subtract: os << "-="; break;
            case token_kind::op_assign_multiply: os << "*="; break;
            case token_kind::op_assign_divide:   os << "/="; break;
            case token_kind::op_assign_modulo:   os << "%="; break;
            case token_kind::op_assign_concat:   os << "@="; break;
            case token_kind::op_increment:       os << "++"; break;
            case token_kind::op_decrement:       os << "--"; break;
            default:                             break;
            }

            return os;
        }

        void write(ostream &os, const unique_ptr<ast::string_expr> &expr) {
            os << '"';
            for (const char ch : expr->value) {
                switch (ch) {
                case '\t': os << "\\t"; break;
                case '\n': os << "\\n"; break;
                case '\r': os << "\\r"; break;
                case '\\': os << "\\"; break;
                case '"':  os << "\\\""; break;
                default:   os << ch; break;
                }
            }
            os << '"';
        }

        void write(ostream &os, const unique_ptr<ast::array_expr> &expr) {
            os << '{';
            for (size_t i = 0; i < expr->elements.size(); ++i) {
                if (i > 0) {
                    os << ", ";
                }

                os << expr->elements[i];
            }
            os << '}';
        }

        void write(ostream &os, const unique_ptr<ast::member_expr> &expr) {
            os << expr->object << '.' << expr->member;
        }

        void write(ostream &os, const unique_ptr<ast::index_expr> &expr) {
            os << expr->object << '[' << expr->index << ']';
        }

        void write(ostream &os, const unique_ptr<ast::call_expr> &expr) {
            os << expr->callee << '(';
            for (size_t i = 0; i < expr->args.size(); ++i) {
                if (i > 0) {
                    os << ", ";
                }
                os << expr->args[i];
            }
            os << ')';
        }

        void write(ostream &os, const unique_ptr<ast::unary_expr> &expr) {
            if (expr->prefix) {
                os << expr->op;
                os << expr->operand;
                return;
            }

            os << expr->operand;
            os << expr->op;
        }

        void write(ostream &os, const unique_ptr<ast::binary_expr> &expr) {
            os << expr->left << ' ' << expr->op << ' ' << expr->right;
        }

        void write(ostream &os, const unique_ptr<ast::ternary_expr> &expr) {
            os << expr->condition << " ? " << expr->then_branch << " : " << expr->else_branch;
        }

        void write(ostream &os, const unique_ptr<ast::assign_expr> &expr) {
            os << expr->target << expr->op << expr->value;
        }

        void write(ostream &os, const unique_ptr<ast::new_expr> &expr) {
            os << "new " << expr->object_name << '(';
            for (size_t i = 0; i < expr->args.size(); ++i) {
                if (i > 0) {
                    os << ", ";
                }
                os << to_string(expr->args[i]);
            }
            os << ')';

            if (!expr->body.empty()) {
                os << "{...}";
            }
        }

        void write(ostream &os, const unique_ptr<ast::range_expr> &expr) {
            os << '|' << expr->min << ", " << expr->max << '|';
        }

        void write(ostream &os, const unique_ptr<ast::in_expr> &expr) {
            os << expr->value << " in " << expr->range;
        }

        void write(ostream &os, const unique_ptr<ast::scope_resolution_expr> &expr) {
            os << expr->scope << "::" << expr->member;
        }

        class printer_impl {
            ostream &out_;
            string prefix_;

          public:
            explicit printer_impl(ostream &out) : out_(out) {
            }

            void print(const stmt &list) {
                print_stmt(list);
            }

            void operator()(const unique_ptr<number_expr> &ex) {
                node(format("Number ({})", ex->value));
            }

            void operator()(const unique_ptr<string_expr> &ex) {
                node(format("String ({})", ex->value));
            }

            void operator()(const unique_ptr<boolean_expr> &ex) {
                node(format("Boolean ({})", ex->value ? "true" : "false"));
            }

            void operator()(const unique_ptr<array_expr> &ex) {
                node(format("Array ({} elements)", ex->elements.size()));
            }

            void operator()(const unique_ptr<null_expr> &ex) {
                node("Null");
            }

            void operator()(const unique_ptr<identifier_expr> &ex) {
                node(format("Identifier ({})", ex->name));
            }

            void operator()(const unique_ptr<member_expr> &ex) {
                node("Member");
                property("object", ex->object);
                property("member", ex->member, true);
            }

            void operator()(const unique_ptr<index_expr> &ex) {
                node("Index");
                property("object", ex->object);
                property("index", ex->index, true);
            }

            void operator()(const unique_ptr<call_expr> &ex) {
                node(format("Call ({} args)", ex->args.size()));
                property("callee", ex->callee, ex->args.empty());
                property_set("arg", ex->args);
            }

            void operator()(const unique_ptr<unary_expr> &ex) {
                node(format("Unary ({}, {})", token_kind_string(ex->op), ex->prefix ? "prefix" : "postfix"));
                property("operand", ex->operand, true);
            }

            void operator()(const unique_ptr<binary_expr> &ex) {
                node(format("Binary ({})", token_kind_string(ex->op)));
                property("left", ex->left);
                property("right", ex->right, true);
            }

            void operator()(const unique_ptr<ternary_expr> &ex) {
                node(format("Ternary"));
                property("condition", ex->condition);
                property("then", ex->then_branch);
                property("else", ex->else_branch, true);
            }

            void operator()(const unique_ptr<assign_expr> &ex) {
                node(format("Assign ({})", token_kind_string(ex->op)));
                property("target", ex->target);
                property("value", ex->value, true);
            }

            void operator()(const unique_ptr<new_expr> &ex) {
                auto has_body = !ex->body.empty();

                node(format("New ({} args{})", ex->args.size(), has_body ? " with body" : ""));
                property("type", ex->object_name, ex->args.empty() && !has_body);
                property_set("arg", ex->args, has_body);

                if (has_body) {
                    body("body", ex->body);
                }
            }

            void operator()(const unique_ptr<range_expr> &ex) {
                node("Range");
                property("min", ex->min);
                property("max", ex->max, true);
            }

            void operator()(const unique_ptr<in_expr> &ex) {
                node("In");
                property("value", ex->value);
                property("range", ex->range, true);
            }

            void operator()(const unique_ptr<scope_resolution_expr> &ex) {
                node("ScopeMember");
                property("scope", ex->scope);
                property("member", ex->member, true);
            }

            void operator()(const unique_ptr<block_stmt> &st) {
                node(format("Block ({} stmts)", st->body.size()));
                print_stmt_list(st->body);
            }

            void operator()(const unique_ptr<expr_stmt> &st) {
                node("Expr");
                property("expr", st->expression, true);
            }

            void operator()(const unique_ptr<if_stmt> &st) {
                const auto has_else = st->else_branch.has_value();

                node("If");
                property("condition", st->condition);
                property("then", st->then_branch, has_else);

                if (has_else) {
                    property("else", *st->else_branch, true);
                }
            }

            void operator()(const unique_ptr<while_stmt> &st) {
                node("While");
                property("condition", st->condition);
                property("body", st->body, true);
            }

            void operator()(const unique_ptr<for_stmt> &st) {
                node("For");

                if (st->initializer) {
                    property("initializer", *st->initializer);
                }

                if (st->condition) {
                    property("condition", *st->condition);
                }

                if (st->post) {
                    property("post", *st->post);
                }

                property("body", st->body, true);
            }

            void operator()(const unique_ptr<foreach_stmt> &st) {
                node("Foreach");
                property("variable", st->variable);
                property("iterable", st->iterable);
                property("body", st->body, true);
            }

            void operator()(const unique_ptr<with_stmt> &st) {
                node("With");
                property("object", st->object);
                property("body", st->body);
            }

            void operator()(const unique_ptr<return_stmt> &st) {
                node("Return");

                if (st->value) {
                    property("value", *st->value, true);
                }
            }

            void operator()(const unique_ptr<break_stmt> &st) {
                node("Break");
            }

            void operator()(const unique_ptr<continue_stmt> &st) {
                node("Continue");
            }

            void operator()(const unique_ptr<function_stmt> &st) {
                string params;
                for (size_t i = 0; i < st->params.size(); ++i) {
                    if (i > 0) {
                        params += ", ";
                    }

                    params += st->params[i];
                }

                node(format("Function {}({})", st->name, params));
                property("body", st->body);
            }

            void operator()(const unique_ptr<switch_stmt> &st) {
                node("Switch");
                property("operand", st->operand, true);
            }

            void operator()(const unique_ptr<enum_stmt> &st) {
                node(format("Enum ({} entries)", st->entries.size()));

                for (size_t i = 0; i < st->entries.size(); ++i) {
                    auto last = (i == st->entries.size() - 1);
                    auto ctx = child(format("{}", st->entries[i].name), last);
                    if (st->entries[i].value) {
                        out_ << " = ";
                        print_expr(*st->entries[i].value);
                    } else {
                        out_ << '\n';
                    }
                }
            }

            void operator()(const unique_ptr<do_while_stmt> &st) {
                node("DoWhile");
                property("condition", st->condition);
                property("body", st->body, true);
            }

          private:
            class child_scope {
                printer_impl &printer_;
                string saved_prefix_;

              public:
                child_scope(printer_impl &print, string_view label, bool last) : printer_(print), saved_prefix_(print.prefix_) {
                    printer_.out_
                        << printer_.prefix_
                        << (last ? "└── " : "├── ")
                        << label;

                    printer_.prefix_ += last ? "    " : "│   ";
                }

                ~child_scope() {
                    printer_.prefix_ = saved_prefix_;
                }
            };

            auto child(string_view label, bool last) -> child_scope {
                return {*this, label, last};
            }

            void node(string_view label) {
                out_ << label << "\n";
            }

            void property(string_view label, const expr &expr, const bool last = false) {
                auto ctx = child(format("{}: ", label), last);
                print_expr(expr);
            }

            void property(string_view label, const stmt &stmt, const bool last = false) {
                auto ctx = child(format("{}: ", label), last);
                print_stmt(stmt);
            }

            void property_set(string_view label, const expr_list &list, bool after_last = false) {
                for (size_t i = 0; i < list.size(); ++i) {
                    auto last = (i == list.size() - 1) && !after_last;
                    auto ctx = child(format("{}{} ", label, i), last);

                    print_expr(list[i]);
                }
            }

            void body(string_view label, const expr_list &list) {
                auto ctx = child(format("{} ({} exprs)", label, list.size()), true);
                out_ << '\n';
                property_set("expr", list);
            }

            void body(string_view label, const stmt_list &list) {
                auto ctx = child(format("{} ({} stmts)", label, list.size()), true);
                out_ << '\n';
                print_stmt_list(list);
            }

            void body(string_view label, const block_stmt &list) {
                if (list.body.empty()) {
                    return;
                }

                auto ctx = child(format("{} ({} stmts)", label, list.body.size()), true);
                out_ << '\n';
                print_stmt_list(list.body);
            }

            void print_expr(const expr &ex) {
                visit(*this, ex);
            }

            void print_stmt(const stmt &st) {
                visit(*this, st);
            }

            void print_stmt_list(const stmt_list &list) {
                for (size_t i = 0; i < list.size(); ++i) {
                    auto ctx = child("", i == list.size() - 1);

                    print_stmt(list[i]);
                }
            }
        };
    }

    void print(std::ostream &os, const stmt &stmt) {
        printer_impl printer(os);

        printer.print(stmt);
    }

    auto to_string(const ast::expr &expr) -> string {
        ostringstream os;

        os << expr;

        return os.str();
    }

    auto operator<<(ostream &os, const ast::expr &expr) -> ostream & {
        visit(
            overloaded{
                [&](const unique_ptr<ast::number_expr> &expr) -> void { os << std::to_string(expr->value); },
                [&](const unique_ptr<ast::string_expr> &expr) -> void { write(os, expr); },
                [&](const unique_ptr<ast::boolean_expr> &expr) -> void { os << (expr->value ? "true" : "false"); },
                [&](const unique_ptr<ast::array_expr> &expr) -> void { write(os, expr); },
                [&](const unique_ptr<ast::null_expr> &expr) -> void { os << "null"; },
                [&](const unique_ptr<ast::identifier_expr> &expr) -> void { os << expr->name; },
                [&](const unique_ptr<ast::member_expr> &expr) -> void { write(os, expr); },
                [&](const unique_ptr<ast::index_expr> &expr) -> void { write(os, expr); },
                [&](const unique_ptr<ast::call_expr> &expr) -> void { write(os, expr); },
                [&](const unique_ptr<ast::unary_expr> &expr) -> void { write(os, expr); },
                [&](const unique_ptr<ast::binary_expr> &expr) -> void { write(os, expr); },
                [&](const unique_ptr<ast::ternary_expr> &expr) -> void { write(os, expr); },
                [&](const unique_ptr<ast::assign_expr> &expr) -> void { write(os, expr); },
                [&](const unique_ptr<ast::new_expr> &expr) -> void { write(os, expr); },
                [&](const unique_ptr<ast::range_expr> &expr) -> void { write(os, expr); },
                [&](const unique_ptr<ast::in_expr> &expr) -> void { write(os, expr); },
                [&](const unique_ptr<ast::scope_resolution_expr> &expr) -> void { write(os, expr); },
            },
            expr);

        return os;
    }
}
