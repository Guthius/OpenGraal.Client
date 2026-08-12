#include <gs2/eval.hpp>

#include <gs2/builtins.hpp>
#include <gs2/prototype.hpp>
#include <gs2/script.hpp>

#include <cmath>
#include <format>

using namespace std;

namespace og::gs2 {
    namespace {
        constexpr value null_value = value{};

        auto to_integer(const value &val) -> int64_t {
            return static_cast<int64_t>(trunc(to_number(val)));
        }
        constexpr value zero_value = value{0.0};
        constexpr value false_value = zero_value;
        constexpr value true_value = value{1.0};

        template <class... Ts>
        struct overloaded : Ts... {
            using Ts::operator()...;
        };

        struct member_ref {
            dictionary_ptr dictionary;
            std::string key;
        };

        struct array_ref {
            array_ptr array;
            size_t index;
        };

        using lvalue = std::variant<member_ref, array_ref>;

        using expected_lvalue = std::expected<lvalue, error>;

        auto resolve_identifier(context &context, const unique_ptr<ast::identifier_expr> &expr) -> expected_lvalue {
            const auto &name = expr->name;

            if (name == "this" || name == "thiso" || name == "temp" || name == "params" || context.find_scope(name)) {
                return unexpected(error{
                    .source = error_source::interpreter,
                    .kind = error_kind::runtime_error,
                    .message = format("'{}' is not assignable", name),
                    .position = expr->position,
                });
            }

            if (context.is_parameter(name)) {
                return member_ref{
                    .dictionary = context.get_temp(),
                    .key = name,
                };
            }

            auto target = context.find_with(name);

            return member_ref{
                .dictionary = target ? target : context.get_locals(),
                .key = name,
            };
        }

        auto resolve_member_key(context &context, const ast::expr &expr) -> expected<string, error> {
            if (const auto *identifier = get_if<unique_ptr<ast::identifier_expr>>(&expr)) {
                return (*identifier)->name;
            }

            auto member = eval(context, expr);
            if (!member) {
                return unexpected(member.error());
            }

            return to_string(*member);
        }

        auto resolve_lvalue(context &context, const ast::expr &expr) -> expected_lvalue;
        void set(const lvalue &lvalue, value new_value);

        auto vivify_object(context &context, const ast::expr &expr, const value &current) -> dictionary_ptr {
            if (const auto *dictionary = get_if<dictionary_ptr>(&current)) {
                return *dictionary;
            }

            if (!holds_alternative<monostate>(current)) {
                return nullptr;
            }

            auto target = resolve_lvalue(context, expr);
            if (!target) {
                return nullptr;
            }

            auto created = make_shared<basic_dictionary>();
            set(*target, value{created});

            return created;
        }

        auto null_teardown(const value &current, const string_view method) -> callable_ptr {
            static constexpr std::array verbs = {"destroy"sv, "hide"sv, "clear"sv, "clearrows"sv, "removeall"sv};

            if (!holds_alternative<monostate>(current) || !ranges::contains(verbs, fold_case(method))) {
                return nullptr;
            }

            struct no_op : callable {
                auto invoke(const values &) -> expected_value override {
                    return value{};
                }
            };

            return make_shared<no_op>();
        }

        auto vivify_array(context &context, const ast::expr &expr, const value &current, const string_view method) -> array_ptr {
            static constexpr std::array builders = {"add"sv, "insert"sv, "addsorted"sv, "loadfolder"sv};

            if (!holds_alternative<monostate>(current) || !ranges::contains(builders, fold_case(method))) {
                return nullptr;
            }

            auto target = resolve_lvalue(context, expr);
            if (!target) {
                return nullptr;
            }

            auto created = make_shared<gs2::array>();
            set(*target, value{created});

            return created;
        }

        auto resolve_member(context &context, const unique_ptr<ast::member_expr> &expr) -> expected_lvalue {
            auto object = eval(context, expr->object);
            if (!object) {
                return unexpected(object.error());
            }

            auto key = resolve_member_key(context, expr->member);
            if (!key) {
                return unexpected(key.error());
            }

            if (auto dictionary = vivify_object(context, expr->object, *object)) {
                return member_ref{
                    .dictionary = std::move(dictionary),
                    .key = std::move(*key),
                };
            }

            return unexpected(error{
                .source = error_source::interpreter,
                .kind = error_kind::runtime_error,
                .message = "member assignment requires an object",
                .position = expr->position,
            });
        }

        auto resolve_index(context &context, const unique_ptr<ast::index_expr> &expr) -> expected_lvalue {
            auto object = eval(context, expr->object);
            if (!object) {
                return unexpected(object.error());
            }

            auto index = eval(context, expr->index);
            if (!index) {
                return unexpected(index.error());
            }

            if (auto *array = get_if<array_ptr>(&*object)) {
                auto index_num = static_cast<size_t>(to_number(*index));
                if (index_num >= (*array)->elements.size()) {
                    (*array)->elements.resize(index_num + 1);
                }

                return array_ref{
                    .array = *array,
                    .index = index_num,
                };
            }

            return unexpected(error{
                .source = error_source::interpreter,
                .kind = error_kind::runtime_error,
                .message = "index target is not an array",
                .position = expr->position,
            });
        }

        auto resolve_named_path(context &context, const string &text, const source_position position) -> expected_lvalue {
            const auto dot = text.rfind('.');

            if (dot == string::npos) {
                if (auto scope = context.find_scope(text)) {
                    return member_ref{.dictionary = std::move(scope), .key = text};
                }

                return member_ref{.dictionary = context.get_temp(), .key = text};
            }

            const auto owner = resolve_path(context, string_view(text).substr(0, dot), true);
            const auto *dictionary = get_if<dictionary_ptr>(&owner);

            if (dictionary == nullptr || !*dictionary) {
                return unexpected(error{
                    .source = error_source::interpreter,
                    .kind = error_kind::runtime_error,
                    .message = std::format("'{}' does not name a variable", text),
                    .position = position,
                });
            }

            return member_ref{.dictionary = *dictionary, .key = text.substr(dot + 1)};
        }

        auto resolve_call(context &context, const unique_ptr<ast::call_expr> &expr) -> expected_lvalue {
            const auto *callee = get_if<unique_ptr<ast::identifier_expr>>(&expr->callee);

            if (callee == nullptr || !folded_equal{}((*callee)->name, "makevar") || expr->args.size() != 1) {
                return unexpected(error{
                    .source = error_source::interpreter,
                    .kind = error_kind::runtime_error,
                    .message = "expression is not assignable",
                    .position = expr->position,
                });
            }

            auto path = eval(context, expr->args.front());
            if (!path) {
                return unexpected(path.error());
            }

            return resolve_named_path(context, to_string(*path), expr->position);
        }

        auto resolve_lvalue(context &context, const ast::expr &expr) -> expected_lvalue {
            return visit(
                overloaded{
                    [&](const unique_ptr<ast::unary_expr> &expr) -> expected_lvalue {
                        if (!expr->prefix || expr->op != token_kind::op_concat) {
                            return unexpected(error{
                                .source = error_source::interpreter,
                                .kind = error_kind::runtime_error,
                                .message = "expression is not assignable",
                                .position = expr->position,
                            });
                        }

                        auto named = eval(context, expr->operand);
                        if (!named) {
                            return unexpected(named.error());
                        }

                        return resolve_named_path(context, to_string(*named), expr->position);
                    },
                    [&](const unique_ptr<ast::identifier_expr> &expr) -> expected_lvalue {
                        return resolve_identifier(context, expr);
                    },
                    [&](const unique_ptr<ast::member_expr> &expr) -> expected_lvalue {
                        return resolve_member(context, expr);
                    },
                    [&](const unique_ptr<ast::index_expr> &expr) -> expected_lvalue {
                        return resolve_index(context, expr);
                    },
                    [&](const unique_ptr<ast::call_expr> &expr) -> expected_lvalue {
                        return resolve_call(context, expr);
                    },
                    [](const auto &) -> expected_lvalue {
                        return unexpected(error{
                            .source = error_source::interpreter,
                            .kind = error_kind::runtime_error,
                            .message = "expression is not assignable",
                        });
                    },
                },
                expr);
        }

        auto get(const lvalue &lvalue) -> value {
            return visit(
                overloaded{
                    [](const member_ref &ref) -> value {
                        return ref.dictionary->get(ref.key);
                    },
                    [](const array_ref &ref) -> value {
                        if (ref.index < ref.array->elements.size()) {
                            return ref.array->elements[ref.index];
                        }

                        return null_value;
                    }},
                lvalue);
        }

        void set(const lvalue &lvalue, value new_value) {
            visit(
                overloaded{
                    [&new_value](const member_ref &ref) -> void {
                        ref.dictionary->put(ref.key, std::move(new_value));
                    },
                    [&new_value](const array_ref &ref) -> void {
                        if (ref.index >= ref.array->elements.size()) {
                            ref.array->elements.resize(ref.index + 1);
                        }

                        ref.array->elements[ref.index] = std::move(new_value);
                    },
                },
                lvalue);
        }

        enum class flow_kind : uint8_t {
            none,
            break_,
            continue_,
            return_,
        };

        struct state {
            flow_kind flow = flow_kind::none;
            value return_value;

            auto consume(flow_kind check) -> bool {
                if (flow != check) {
                    return false;
                }
                flow = flow_kind::none;
                return true;
            }
        };

        auto eval(context &context, state &state, const ast::stmt &stmt) -> expected_void;

        auto eval_array(context &context, const unique_ptr<ast::array_expr> &expr) -> expected_value {
            auto arr = make_shared<array>();
            for (const auto &element : expr->elements) {
                auto element_value = eval(context, element);
                if (!element_value) {
                    return element_value;
                }

                arr->elements.push_back(std::move(*element_value));
            }

            return value{arr};
        }

        auto eval_identifier(context &context, const unique_ptr<ast::identifier_expr> &expr) -> expected_value {
            return context.get(expr->name);
        }

        auto member_value(context &context, const value &object, const string &key) -> expected_value {
            if (const auto *dict = get_if<dictionary_ptr>(&object)) {
                if ((*dict)->contains(key)) {
                    return (*dict)->get(key);
                }
            }

            if (auto method = find_method(context.get_environment(), object, key)) {
                return value{std::move(method)};
            }

            if (holds_alternative<dictionary_ptr>(object)) {
                return null_value;
            }

            if (const auto *name = get_if<string>(&object)) {
                if (auto named = context.get_environment().find_object(*name)) {
                    return member_value(context, value{std::move(named)}, key);
                }
            }

            return object;
        }

        auto eval_member(context &context, const unique_ptr<ast::member_expr> &expr) -> expected_value {
            auto object = eval(context, expr->object);
            if (!object) {
                return object;
            }

            auto key = resolve_member_key(context, expr->member);
            if (!key) {
                return unexpected(key.error());
            }

            return member_value(context, *object, *key);
        }

        auto eval_callee(context &context, const ast::expr &callee) -> expected_value {
            const auto *member = get_if<unique_ptr<ast::member_expr>>(&callee);
            if (member == nullptr) {
                return eval(context, callee);
            }

            auto object = eval(context, (*member)->object);
            if (!object) {
                return object;
            }

            auto key = resolve_member_key(context, (*member)->member);
            if (!key) {
                return unexpected(key.error());
            }

            auto found = member_value(context, *object, *key);
            if (!found || holds_alternative<callable_ptr>(*found)) {
                return found;
            }

            if (auto ignored = null_teardown(*object, *key)) {
                return value{std::move(ignored)};
            }

            if (auto created = vivify_array(context, (*member)->object, *object, *key)) {
                return member_value(context, value{std::move(created)}, *key);
            }

            if (auto created = vivify_object(context, (*member)->object, *object)) {
                return member_value(context, value{std::move(created)}, *key);
            }

            return found;
        }

        auto eval_index(context &context, const unique_ptr<ast::index_expr> &expr) -> expected_value {
            auto object = eval(context, expr->object);
            if (!object) {
                return object;
            }

            auto index = eval(context, expr->index);
            if (!index) {
                return index;
            }

            if (auto *arr = get_if<array_ptr>(&*object)) {
                auto idx = static_cast<size_t>(to_number(*index));
                if (idx < (*arr)->elements.size()) {
                    return (*arr)->elements[idx];
                }

                return null_value;
            }

            if (auto *dict = get_if<dictionary_ptr>(&*object)) {
                auto key = to_string(*index);
                if ((*dict)->contains(key)) {
                    return (*dict)->get(key);
                }

                return null_value;
            }

            return null_value;
        }

        auto eval_call(context &context, const unique_ptr<ast::call_expr> &expr) -> expected_value {
            auto callee = eval_callee(context, expr->callee);
            if (!callee) {
                return callee;
            }

            values args;
            for (const auto &val : expr->args) {
                auto arg = eval(context, val);
                if (!arg) {
                    return arg;
                }

                args.push_back(std::move(*arg));
            }

            if (auto *callable = get_if<callable_ptr>(&*callee)) {
                if (auto *with_context = dynamic_cast<context_callable *>(callable->get())) {
                    return with_context->invoke_with(context, args);
                }

                return (*callable)->invoke(args);
            }

            return unexpected(error{
                .source = error_source::interpreter,
                .kind = error_kind::runtime_error,
                .message = format("expression '{}' is not callable", to_string(expr->callee)),
                .position = expr->position,
            });
        }

        auto eval_unary(context &context, const unique_ptr<ast::unary_expr> &expr) -> expected_value {
            if (expr->prefix) {
                if (expr->op == token_kind::op_increment ||
                    expr->op == token_kind::op_decrement) {
                    auto lvalue = resolve_lvalue(context, expr->operand);
                    if (!lvalue) {
                        return unexpected(lvalue.error());
                    }

                    auto old_value = to_number(get(*lvalue));
                    auto new_value = expr->op == token_kind::op_increment ? old_value + 1 : old_value - 1;

                    set(*lvalue, value{new_value});

                    return value{new_value};
                }

                auto operand = eval(context, expr->operand);
                switch (expr->op) {
                case token_kind::op_not:
                    return to_bool(*operand) ? false_value : true_value;

                case token_kind::op_subtract:
                    return value{-to_number(*operand)};

                case token_kind::op_bitnot:
                    return value{static_cast<double>(~to_integer(*operand))};

                case token_kind::op_concat:
                    return resolve_path(context, to_string(*operand));

                default:
                    return null_value;
                }
            }

            auto lvalue = resolve_lvalue(context, expr->operand);
            if (!lvalue) {
                return unexpected(lvalue.error());
            }

            auto old_value = to_number(get(*lvalue));
            auto new_value = expr->op == token_kind::op_increment ? old_value + 1 : old_value - 1;

            set(*lvalue, value{new_value});

            return value{old_value};
        }

        auto eval_logical_and(context &context, const unique_ptr<ast::binary_expr> &expr) -> expected_value {
            auto left = eval(context, expr->left);
            if (!left) {
                return left;
            }

            if (!to_bool(*left)) {
                return false_value;
            }

            auto right = eval(context, expr->right);
            if (!right) {
                return right;
            }

            return to_bool(*right) ? true_value : false_value;
        }

        auto eval_logical_or(context &context, const unique_ptr<ast::binary_expr> &expr) -> expected_value {
            auto left = eval(context, expr->left);
            if (!left) {
                return left;
            }

            if (to_bool(*left)) {
                return true_value;
            }

            auto right = eval(context, expr->right);
            if (!right) {
                return right;
            }

            return to_bool(*right) ? true_value : false_value;
        }

        auto eval_binary(context &context, const unique_ptr<ast::binary_expr> &expr) -> expected_value {
            if (expr->op == token_kind::op_and) {
                return eval_logical_and(context, expr);
            }

            if (expr->op == token_kind::op_or) {
                return eval_logical_or(context, expr);
            }

            auto left = eval(context, expr->left);
            if (!left) {
                return left;
            }

            auto right = eval(context, expr->right);
            if (!right) {
                return right;
            }

            switch (expr->op) {
            case token_kind::op_add:        return value{to_number(*left) + to_number(*right)};
            case token_kind::op_subtract:   return value{to_number(*left) - to_number(*right)};
            case token_kind::op_multiply:   return value{to_number(*left) * to_number(*right)};
            case token_kind::op_exponent:   return value(pow(to_number(*left), to_number(*right)));
            case token_kind::op_concat:     return value(to_string(*left) + to_string(*right));
            case token_kind::op_concat_spc: return value(to_string(*left) + " " + to_string(*right));
            case token_kind::op_concat_nl:  return value(to_string(*left) + "\n" + to_string(*right));
            case token_kind::op_eq:         return values_equal(*left, *right) ? true_value : false_value;
            case token_kind::op_neq:        return values_equal(*left, *right) ? false_value : true_value;
            case token_kind::op_lt:         return to_number(*left) < to_number(*right) ? true_value : false_value;
            case token_kind::op_lte:        return to_number(*left) <= to_number(*right) ? true_value : false_value;
            case token_kind::op_gt:         return to_number(*left) > to_number(*right) ? true_value : false_value;
            case token_kind::op_gte:        return to_number(*left) >= to_number(*right) ? true_value : false_value;

            case token_kind::pipe:      return value{static_cast<double>(to_integer(*left) | to_integer(*right))};
            case token_kind::op_bitand: return value{static_cast<double>(to_integer(*left) & to_integer(*right))};
            case token_kind::op_xor:    return value{static_cast<double>(to_integer(*left) ^ to_integer(*right))};
            case token_kind::op_shl:    return value{static_cast<double>(to_integer(*left) << to_integer(*right))};
            case token_kind::op_shr:    return value{static_cast<double>(to_integer(*left) >> to_integer(*right))};

            case token_kind::op_divide:
                {
                    auto divisor = to_number(*right);
                    if (divisor == 0.0) {
                        return zero_value;
                    }

                    return value{to_number(*left) / divisor};
                }

            case token_kind::op_modulo:
                {
                    auto divisor = to_number(*right);
                    if (divisor == 0.0) {
                        return zero_value;
                    }

                    return value{fmod(to_number(*left), divisor)};
                }

            default: return null_value;
            }
        }

        auto eval_ternary(context &context, const unique_ptr<ast::ternary_expr> &expr) -> expected_value {
            auto condition = eval(context, expr->condition);
            if (!condition) {
                return condition;
            }

            return to_bool(*condition)
                       ? eval(context, expr->then_branch)
                       : eval(context, expr->else_branch);
        }

        auto eval_assign(context &context, const unique_ptr<ast::assign_expr> &expr) -> expected_value {
            auto lvalue = resolve_lvalue(context, expr->target);
            if (!lvalue) {
                return unexpected(lvalue.error());
            }

            auto right = eval(context, expr->value);
            if (!right) {
                return right;
            }

            value result;
            if (expr->op == token_kind::op_assign) {
                result = *right;
            } else {
                auto current_value = to_number(get(*lvalue));
                auto new_value = to_number(*right);

                switch (expr->op) {
                case token_kind::op_assign_add:      result = value{current_value + new_value}; break;
                case token_kind::op_assign_subtract: result = value{current_value - new_value}; break;
                case token_kind::op_assign_multiply: result = value{current_value * new_value}; break;
                case token_kind::op_assign_divide:   result = value{new_value != 0 ? current_value / new_value : 0}; break;
                case token_kind::op_assign_modulo:   result = value{new_value != 0 ? fmod(current_value, new_value) : 0}; break;
                case token_kind::op_assign_exponent: result = value{pow(current_value, new_value)}; break;

                case token_kind::op_assign_bitand: result = value{static_cast<double>(to_integer(get(*lvalue)) & to_integer(*right))}; break;
                case token_kind::op_assign_bitor:  result = value{static_cast<double>(to_integer(get(*lvalue)) | to_integer(*right))}; break;
                case token_kind::op_assign_shl:    result = value{static_cast<double>(to_integer(get(*lvalue)) << to_integer(*right))}; break;
                case token_kind::op_assign_shr:    result = value{static_cast<double>(to_integer(get(*lvalue)) >> to_integer(*right))}; break;

                case token_kind::op_assign_concat:
                    result = value{to_string(get(*lvalue)) + to_string(*right)};
                    break;

                default:
                    result = *right;
                }
            }

            set(*lvalue, result);

            return result;
        }

        auto eval_new(context &context, const unique_ptr<ast::new_expr> &expr) -> expected_value {
            auto type_name = resolve_member_key(context, expr->object_name);
            if (!type_name) {
                return unexpected(type_name.error());
            }

            values args;
            for (const auto &arg : expr->args) {
                auto arg_value = eval(context, arg);
                if (!arg_value) {
                    return arg_value;
                }

                if (holds_alternative<monostate>(*arg_value)) {
                    if (const auto *identifier = get_if<unique_ptr<ast::identifier_expr>>(&arg)) {
                        *arg_value = value{(*identifier)->name};
                    }
                }

                args.push_back(std::move(*arg_value));
            }

            if (*type_name == "Array") {
                auto elements = make_shared<array>();
                if (!args.empty()) {
                    elements->elements.resize(static_cast<size_t>(to_number(args.front())));
                }

                return value{elements};
            }

            if (folded_equal{}(*type_name, "TStaticVar")) {
                return value{make_shared<basic_dictionary>()};
            }

            auto proto = context.get_environment().find_type(*type_name);
            if (!proto) {
                return unexpected(error{
                    .source = error_source::interpreter,
                    .kind = error_kind::runtime_error,
                    .message = format("unknown type '{}'", *type_name),
                    .position = expr->position,
                });
            }

            if (!proto->factory) {
                return unexpected(error{
                    .source = error_source::interpreter,
                    .kind = error_kind::runtime_error,
                    .message = format("type '{}' cannot be constructed", *type_name),
                    .position = expr->position,
                });
            }

            auto instance = proto->factory(args);
            if (!instance) {
                return unexpected(error{
                    .source = error_source::interpreter,
                    .kind = error_kind::runtime_error,
                    .message = format("could not construct '{}'", *type_name),
                    .position = expr->position,
                });
            }

            auto object = make_shared<prototype_dictionary>(proto, std::move(instance));

            context.get_environment().object_created(object, context.get_dictionary());

            if (!args.empty()) {
                if (const auto *name = get_if<string>(&args.front())) {
                    context.get_environment().register_object(*name, object);
                }
            }

            if (!expr->body.empty()) {
                auto self = context.push_self(object);
                auto locals = context.push_locals(object);

                state body_state;

                for (const auto &item : expr->body) {
                    auto result = eval(context, body_state, item);
                    if (!result) {
                        return unexpected(result.error());
                    }

                    if (body_state.flow != flow_kind::none) {
                        break;
                    }
                }
            }

            return value{object};
        }

        auto eval_in(context &context, const unique_ptr<ast::in_expr> &expr) -> expected_value {
            auto value = eval(context, expr->value);
            if (!value) {
                return value;
            }

            if (auto *range = get_if<unique_ptr<ast::range_expr>>(&expr->range)) {
                auto min = eval(context, (*range)->min);
                if (!min) {
                    return min;
                }

                auto max = eval(context, (*range)->max);
                if (!max) {
                    return max;
                }

                const auto num = to_number(*value);
                const auto low = to_number(*min);
                const auto high = to_number(*max);

                const auto above = (*range)->min_inclusive ? num >= low : num > low;
                const auto below = (*range)->max_inclusive ? num <= high : num < high;

                return above && below ? true_value : false_value;
            }

            auto right = eval(context, expr->range);
            if (!right) {
                return right;
            }

            if (auto *array = get_if<array_ptr>(&*right)) {
                for (const auto &element : (*array)->elements) {
                    if (values_equal(*value, element)) {
                        return true_value;
                    }
                }
            }

            return false_value;
        }

        struct class_scope_call : callable {
            script_ptr owner;
            dictionary_ptr self;
            string function;

            class_scope_call(script_ptr owner, dictionary_ptr self, string function)
                : owner(std::move(owner)), self(std::move(self)), function(std::move(function)) {
            }

            auto invoke(const values &args) -> expected_value override {
                return owner->call_public(function, self, args);
            }
        };

        auto eval_scope_resolution(context &context, const unique_ptr<ast::scope_resolution_expr> &expr) -> expected_value {
            auto member_name = resolve_member_key(context, expr->member);
            if (!member_name) {
                return unexpected(member_name.error());
            }

            if (const auto *identifier = get_if<unique_ptr<ast::identifier_expr>>(&expr->scope)) {
                if (auto scope = context.find_scope((*identifier)->name)) {
                    return scope->get(*member_name);
                }

                if (auto joined = context.get_environment().find_class((*identifier)->name)) {
                    return value{make_shared<class_scope_call>(std::move(joined), context.get_dictionary(), *member_name)};
                }
            }

            auto scope = eval(context, expr->scope);
            if (!scope) {
                return unexpected(scope.error());
            }

            if (const auto *dictionary = get_if<dictionary_ptr>(&*scope); dictionary != nullptr && *dictionary) {
                return (*dictionary)->get(*member_name);
            }

            return unexpected(error{
                .source = error_source::interpreter,
                .kind = error_kind::runtime_error,
                .message = format("unknown scope '{}'", to_string(expr->scope)),
                .position = expr->position,
            });
        }
    }

    auto eval(context &context, const ast::expr &expr) -> expected_value {
        return visit(
            overloaded{
                [](const unique_ptr<ast::number_expr> &expr) -> expected_value { return value{expr->value}; },
                [](const unique_ptr<ast::string_expr> &expr) -> expected_value { return value{expr->value}; },
                [](const unique_ptr<ast::boolean_expr> &expr) -> expected_value { return value{expr->value ? 1.0 : 0.0}; },
                [](const unique_ptr<ast::null_expr> &expr) -> expected_value { return null_value; },

                [&](const unique_ptr<ast::array_expr> &expr) -> expected_value { return eval_array(context, expr); },
                [&](const unique_ptr<ast::identifier_expr> &expr) -> expected_value { return eval_identifier(context, expr); },
                [&](const unique_ptr<ast::member_expr> &expr) -> expected_value { return eval_member(context, expr); },
                [&](const unique_ptr<ast::index_expr> &expr) -> expected_value { return eval_index(context, expr); },
                [&](const unique_ptr<ast::call_expr> &expr) -> expected_value { return eval_call(context, expr); },
                [&](const unique_ptr<ast::unary_expr> &expr) -> expected_value { return eval_unary(context, expr); },
                [&](const unique_ptr<ast::binary_expr> &expr) -> expected_value { return eval_binary(context, expr); },
                [&](const unique_ptr<ast::ternary_expr> &expr) -> expected_value { return eval_ternary(context, expr); },
                [&](const unique_ptr<ast::assign_expr> &expr) -> expected_value { return eval_assign(context, expr); },
                [&](const unique_ptr<ast::new_expr> &expr) -> expected_value { return eval_new(context, expr); },
                [&](const unique_ptr<ast::range_expr> &expr) -> expected_value { return null_value; },
                [&](const unique_ptr<ast::in_expr> &expr) -> expected_value { return eval_in(context, expr); },
                [&](const unique_ptr<ast::scope_resolution_expr> &expr) -> expected_value { return eval_scope_resolution(context, expr); },
            },
            expr);
    }

    namespace {

        auto eval_block(context &context, state &state, const unique_ptr<ast::block_stmt> &stmt) -> expected_void {
            for (const auto &child : stmt->body) {
                auto result = eval(context, state, child);
                if (!result) {
                    return result;
                }

                if (state.flow != flow_kind::none) {
                    return {};
                }
            }

            return {};
        };

        auto eval_expr(context &context, state &state, const unique_ptr<ast::expr_stmt> &stmt) -> expected_void {
            auto result = eval(context, stmt->expression);
            if (!result) {
                return unexpected(result.error());
            }

            return {};
        }

        auto eval_if(context &context, state &state, const unique_ptr<ast::if_stmt> &stmt) -> expected_void {
            auto condition = eval(context, stmt->condition);
            if (!condition) {
                return unexpected(condition.error());
            }

            if (to_bool(*condition)) {
                return eval(context, state, stmt->then_branch);
            }

            if (stmt->else_branch) {
                return eval(context, state, *stmt->else_branch);
            }

            return {};
        }

        auto eval_while(context &context, state &state, const unique_ptr<ast::while_stmt> &stmt) -> expected_void {
            while (true) {
                auto condition = eval(context, stmt->condition);
                if (!condition) {
                    return unexpected(condition.error());
                }

                if (!to_bool(*condition)) {
                    break;
                }

                auto result = eval(context, state, stmt->body);
                if (!result) {
                    return result;
                }

                if (state.consume(flow_kind::break_)) {
                    break;
                }

                if (state.consume(flow_kind::continue_)) {
                    continue;
                    ;
                }

                if (state.flow == flow_kind::return_) {
                    return {};
                }
            }

            return {};
        }

        auto eval_for(context &context, state &state, const unique_ptr<ast::for_stmt> &stmt) -> expected_void {
            if (stmt->initializer) {
                auto result = eval(context, *stmt->initializer);
                if (!result) {
                    return unexpected(result.error());
                }
            }

            while (true) {
                if (stmt->condition) {
                    auto condition = eval(context, *stmt->condition);
                    if (!condition) {
                        return unexpected(condition.error());
                    }

                    if (!to_bool(*condition)) {
                        break;
                    }
                }

                auto result = eval(context, state, stmt->body);
                if (!result) {
                    return result;
                }

                if (state.consume(flow_kind::break_)) {
                    break;
                }

                if (state.consume(flow_kind::continue_)) {
                    continue;
                }

                if (state.flow == flow_kind::return_) {
                    return {};
                }

                if (stmt->post) {
                    auto result = eval(context, *stmt->post);
                    if (!result) {
                        return unexpected(result.error());
                    }
                }
            }

            return {};
        }

        auto eval_foreach(context &context, state &state, const unique_ptr<ast::foreach_stmt> &stmt) -> expected_void {
            auto iterable = eval(context, stmt->iterable);
            if (!iterable) {
                return unexpected(iterable.error());
            }

            auto lvalue = resolve_lvalue(context, stmt->variable);
            if (!lvalue) {
                return unexpected(lvalue.error());
            }

            auto iterate = [&](const vector<value> &elements) -> expected_void {
                for (const auto &element : elements) {
                    set(*lvalue, element);

                    auto result = eval(context, state, stmt->body);
                    if (!result) {
                        return result;
                    }

                    if (state.consume(flow_kind::break_)) {
                        break;
                    }

                    if (state.consume(flow_kind::continue_)) {
                        continue;
                    }

                    if (state.flow == flow_kind::return_) {
                        return {};
                    }
                }

                return {};
            };

            if (auto *array = get_if<array_ptr>(&*iterable)) {
                return iterate((*array)->elements);
            }

            return {};
        }

        auto eval_with(context &context, state &state, const unique_ptr<ast::with_stmt> &stmt) -> expected_void {
            auto object = eval(context, stmt->object);
            if (!object) {
                return unexpected(object.error());
            }

            auto *dictionary = get_if<dictionary_ptr>(&*object);
            if (dictionary == nullptr || !*dictionary) {
                return {};
            }

            auto self = context.push_self(*dictionary);
            auto target = context.push_with(*dictionary);

            return eval(context, state, stmt->body);
        }

        auto eval_return(context &context, state &state, const unique_ptr<ast::return_stmt> &stmt) -> expected_void {
            if (stmt->value) {
                auto value = eval(context, *stmt->value);
                if (!value) {
                    return unexpected(value.error());
                }

                state.return_value = std::move(*value);
            }

            state.flow = flow_kind::return_;

            return {};
        }

        auto eval_break(context &context, state &state, const unique_ptr<ast::break_stmt> &stmt) -> expected_void {
            state.flow = flow_kind::break_;

            return {};
        }

        auto eval_continue(context &context, state &state, const unique_ptr<ast::continue_stmt> &stmt) -> expected_void {
            state.flow = flow_kind::continue_;

            return {};
        }

        auto eval_switch_at(context &context, state &state, const unique_ptr<ast::switch_stmt> &stmt, size_t index) -> expected_void {
            if (index >= stmt->items.size()) {
                return {};
            }

            for (size_t i = index; i < stmt->items.size(); ++i) {
                const auto &item = stmt->items[i];

                if (holds_alternative<ast::switch_stmt::label>(item)) {
                    continue;
                }

                auto result = eval(context, state, get<ast::stmt>(item));
                if (!result) {
                    return result;
                }

                if (state.consume(flow_kind::break_) || state.flow != flow_kind::none) {
                    return {};
                }
            }

            return {};
        }

        auto eval_switch(context &context, state &state, const unique_ptr<ast::switch_stmt> &stmt) -> expected_void {
            auto operand = eval(context, stmt->operand);
            if (!operand) {
                return unexpected(operand.error());
            }

            size_t default_index = numeric_limits<size_t>::max();

            for (size_t i = 0; i < stmt->items.size(); ++i) {
                const auto &item = stmt->items[i];

                if (const auto *label = get_if<ast::switch_stmt::label>(&item)) {
                    if (!label->value) {
                        default_index = i;
                        continue;
                    }

                    auto value = eval(context, *label->value);
                    if (!value) {
                        return unexpected(value.error());
                    }

                    if (values_equal(*operand, *value)) {
                        return eval_switch_at(context, state, stmt, i + 1);
                    }
                }
            }

            return eval_switch_at(context, state, stmt, default_index);
        }

        auto eval_enum(context &context, state &state, const unique_ptr<ast::enum_stmt> &stmt) -> expected_void {
            auto dictionary = [&]() -> dictionary_ptr {
                if (!stmt->name) {
                    return context.get_dictionary();
                }

                const auto new_dictionary = make_shared<basic_dictionary>();

                context.get_dictionary()->put(*stmt->name, new_dictionary);

                return new_dictionary;
            }();

            auto counter = 0.0;

            for (const auto &entry : stmt->entries) {
                value entry_value;

                if (entry.value) {
                    auto result = eval(context, *entry.value);
                    if (!result) {
                        return unexpected(result.error());
                    }

                    entry_value = *result;
                    if (auto *actual_value = get_if<double>(&entry_value)) {
                        counter = *actual_value + 1;
                    }
                } else {
                    entry_value = value{counter++};
                }

                dictionary->put(entry.name, entry_value);
            }

            return {};
        }

        auto eval_do_while(context &context, state &state, const unique_ptr<ast::do_while_stmt> &stmt) -> expected_void {
            do {
                auto result = eval(context, state, stmt->body);
                if (!result) {
                    return result;
                }

                if (state.consume(flow_kind::break_)) {
                    break;
                }

                if (state.consume(flow_kind::continue_)) {
                }

                if (state.flow == flow_kind::return_) {
                    return {};
                }

                auto condition = eval(context, stmt->condition);
                if (!condition) {
                    return unexpected(condition.error());
                }

                if (!to_bool(*condition)) {
                    break;
                }
            } while (true);

            return {};
        }

        auto eval(context &context, state &state, const ast::stmt &stmt) -> expected_void {
            return visit(
                overloaded{
                    [&](const unique_ptr<ast::block_stmt> &stmt) -> expected_void { return eval_block(context, state, stmt); },
                    [&](const unique_ptr<ast::expr_stmt> &stmt) -> expected_void { return eval_expr(context, state, stmt); },
                    [&](const unique_ptr<ast::if_stmt> &stmt) -> expected_void { return eval_if(context, state, stmt); },
                    [&](const unique_ptr<ast::while_stmt> &stmt) -> expected_void { return eval_while(context, state, stmt); },
                    [&](const unique_ptr<ast::for_stmt> &stmt) -> expected_void { return eval_for(context, state, stmt); },
                    [&](const unique_ptr<ast::foreach_stmt> &stmt) -> expected_void { return eval_foreach(context, state, stmt); },
                    [&](const unique_ptr<ast::with_stmt> &stmt) -> expected_void { return eval_with(context, state, stmt); },
                    [&](const unique_ptr<ast::return_stmt> &stmt) -> expected_void { return eval_return(context, state, stmt); },
                    [&](const unique_ptr<ast::break_stmt> &stmt) -> expected_void { return eval_break(context, state, stmt); },
                    [&](const unique_ptr<ast::continue_stmt> &stmt) -> expected_void { return eval_continue(context, state, stmt); },
                    [&](const unique_ptr<ast::function_stmt> &stmt) -> expected_void { return {}; },
                    [&](const unique_ptr<ast::switch_stmt> &stmt) -> expected_void { return eval_switch(context, state, stmt); },
                    [&](const unique_ptr<ast::enum_stmt> &stmt) -> expected_void { return eval_enum(context, state, stmt); },
                    [&](const unique_ptr<ast::do_while_stmt> &stmt) -> expected_void { return eval_do_while(context, state, stmt); },
                },
                stmt);
        }
    }

    auto eval(context &context, const ast::stmt &stmt) -> expected_void {
        state exec_state{};

        return eval(context, exec_state, stmt);
    }

    auto eval(context &context, const ast::function_stmt &fn, const values &args) -> expected_value {
        if (context.get_depth() >= max_call_depth) {
            return unexpected(error{
                .source = error_source::interpreter,
                .kind = error_kind::runtime_error,
                .message = format("call stack exceeded {} frames calling '{}'", max_call_depth, fn.name),
                .position = fn.position,
            });
        }

        auto temp = make_shared<basic_dictionary>();
        for (size_t i = 0; i < fn.params.size(); ++i) {
            temp->put(fn.params[i], i < args.size() ? args[i] : value{});
        }

        auto params = make_shared<array>();
        params->elements = args;

        auto frame = context.push_frame(std::move(temp), value{params}, fn.params);

        state exec_state{};

        auto result = eval(context, exec_state, fn.body);
        if (!result) {
            return unexpected(result.error());
        }

        if (exec_state.flow == flow_kind::return_) {
            return std::move(exec_state.return_value);
        }

        return {};
    }
}
