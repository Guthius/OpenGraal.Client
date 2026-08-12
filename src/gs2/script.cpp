#include <gs2/script.hpp>

#include <gs2/ast.hpp>
#include <gs2/context.hpp>
#include <gs2/eval.hpp>
#include <gs2/parser.hpp>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>

using namespace std;

namespace og::gs2 {
    namespace {
        struct joined_callable : callable {
            script *owner;
            dictionary_ptr self;
            string function_name;

            joined_callable(script *owner, dictionary_ptr self, string function_name)
                : owner(owner), self(std::move(self)), function_name(std::move(function_name)) {
            }

            auto invoke(const values &args) -> expected_value override {
                return owner->call(function_name, self, args);
            }
        };

        struct script_callable : callable {
            context &ctx;
            const ast::function_stmt *function;

            script_callable(context &ctx, const ast::function_stmt *function)
                : ctx(ctx), function(function) {
            }

            auto invoke(const values &args) -> expected_value override {
                return eval(ctx, *function, args);
            }
        };

        struct script_impl : script {
            using function_stmt_ptr = const ast::function_stmt *;

            environment &env;
            ast::stmt root;
            folded_registry<function_stmt_ptr> functions;
            dictionary_ptr locals = make_shared<basic_dictionary>();
            string name;
            vector<script_ptr> joined;

            script_impl(environment &env, ast::stmt stmt) : env(env), root(std::move(stmt)) {
                detect_functions(root);
            }

            void set_name(string value) override {
                name = std::move(value);
            }

            auto get_name() const -> const string & override {
                return name;
            }

            auto join(const script_ptr &other) -> bool override {
                if (!other || other.get() == this || has_joined(other->get_name())) {
                    return false;
                }

                joined.push_back(other);

                return true;
            }

            auto has_joined(const string_view class_name) const -> bool override {
                return ranges::any_of(joined, [class_name](const script_ptr &other) {
                    return other->get_name() == class_name;
                });
            }

            auto is_public(string_view function_name) const -> bool override {
                if (const auto *function_ptr = get_function(function_name); function_ptr != nullptr) {
                    return function_ptr->is_public;
                }

                return ranges::any_of(joined, [function_name](const script_ptr &other) {
                    return other->is_public(function_name);
                });
            }

            auto has_function(string_view function_name) const -> bool override {
                if (functions.contains(function_name)) {
                    return true;
                }

                return ranges::any_of(joined, [function_name](const script_ptr &other) {
                    return other->has_function(function_name);
                });
            }

            auto find_owner(string_view function_name) -> script * {
                for (auto &other : ranges::reverse_view(joined)) {
                    if (other->has_function(function_name)) {
                        return other.get();
                    }
                }

                return functions.contains(function_name) ? this : nullptr;
            }

            auto get_function(string_view name) const -> function_stmt_ptr {
                auto iter = functions.find(name);
                if (iter == functions.end()) {
                    return nullptr;
                }

                return iter->second;
            }

            auto call(string_view function_name, dictionary_ptr self, const values &args = {}) -> expected_value override {
                return call_with(function_name, std::move(self), locals, args);
            }

            auto call_with(string_view function_name, dictionary_ptr self, dictionary_ptr scope, const values &args = {}) -> expected_value override {
                if (auto *owner = find_owner(function_name); owner != nullptr && owner != this) {
                    return owner->call_with(function_name, std::move(self), std::move(scope), args);
                }

                const auto *function_ptr = get_function(function_name);
                if (function_ptr == nullptr) {
                    return {};
                }

                auto ctx = context(env, std::move(self), scope ? std::move(scope) : locals);
                ctx.set_source_name(name);
                ctx.set_script(this);
                ctx.set_function_resolver([this](context &owner, const string_view function) -> optional<value> {
                    if (auto *script_owner = find_owner(function); script_owner != nullptr && script_owner != this) {
                        return value{make_shared<joined_callable>(script_owner, owner.get_dictionary(), string(function))};
                    }

                    const auto *match = get_function(function);
                    if (match == nullptr) {
                        return nullopt;
                    }

                    return value{make_shared<script_callable>(owner, match)};
                });

                auto result = eval(ctx, *function_ptr, args);
                if (!result) {
                    auto failure = result.error();

                    if (failure.source_name.empty()) {
                        failure.source_name = name;
                    }

                    if (failure.function_name.empty()) {
                        failure.function_name = function_ptr->name;
                    }

                    return unexpected(std::move(failure));
                }

                return result;
            }

            auto call_public(string_view function_name, dictionary_ptr self, const values &args = {}) -> expected_value override {
                if (!is_public(function_name)) {
                    return unexpected(error{
                        .source = error_source::interpreter,
                        .kind = error_kind::runtime_error,
                        .message = format("'{}' is not a public function", function_name),
                        .source_name = name,
                    });
                }

                return call(function_name, std::move(self), args);
            }

            void detect_functions(const ast::stmt &stmt) {
                visit(
                    [this](const auto &node_ptr) -> auto {
                        using T = decay_t<decltype(node_ptr)>;

                        if constexpr (is_same_v<T, unique_ptr<ast::block_stmt>>) {
                            for (const auto &child : node_ptr->body) {
                                detect_functions(child);
                            }
                        }

                        if constexpr (is_same_v<T, unique_ptr<ast::function_stmt>>) {
                            auto key = node_ptr->name;

                            functions[key] = node_ptr.get();
                        }
                    },
                    stmt);
            }
        };
    }

    auto load_script(environment &env, const tokens &tokens) -> script_result {
        auto stmt = parse(tokens);
        if (!stmt) {
            return unexpected(stmt.error());
        }

        return make_shared<script_impl>(env, std::move(*stmt));
    }

    auto load_script(environment &env, istream &is) -> script_result {
        auto tokens = tokenize(is);
        if (!tokens) {
            return unexpected(tokens.error());
        }

        return load_script(env, *tokens);
    }

    auto split_script(const string_view source) -> script_halves {
        constexpr string_view marker = "//#CLIENTSIDE";

        script_halves halves;

        auto in_client = false;
        size_t start = 0;

        while (start < source.size()) {
            const auto end = source.find('\n', start);
            const auto line = end == string_view::npos ? source.substr(start) : source.substr(start, end - start);

            const auto first = line.find_first_not_of(" \t\r");
            const auto trimmed = first == string_view::npos ? string_view{} : line.substr(first);

            if (!in_client && trimmed.starts_with(marker)) {
                in_client = true;
            } else if (in_client) {
                halves.client.append(line);
            } else {
                halves.server.append(line);
            }

            halves.server.push_back('\n');
            halves.client.push_back('\n');

            if (end == string_view::npos) {
                break;
            }

            start = end + 1;
        }

        return halves;
    }

    auto compile(environment &env, const filesystem::path &path) -> script_result {
        if (!filesystem::exists(path)) {
            return unexpected(error{
                .source = error_source::interpreter,
                .kind = error_kind::file_error,
                .message = format("file '{}' does not exist", path.string()),
            });
        }

        if (!filesystem::is_regular_file(path)) {
            return unexpected(error{
                .source = error_source::interpreter,
                .kind = error_kind::file_error,
                .message = format("'{}' is not a file", path.string()),
            });
        }

        ifstream ifs(path, ios::in);
        if (!ifs.is_open()) {
            return unexpected(error{
                .source = error_source::interpreter,
                .kind = error_kind::file_error,
                .message = format("error opening '{}'", path.string()),
            });
        }

        auto script = load_script(env, ifs);
        if (script) {
            (*script)->set_name(path.filename().string());
        }

        return script;
    }
}
