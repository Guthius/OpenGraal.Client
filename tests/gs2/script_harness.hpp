#pragma once

#include <gs2/basic_dictionary.hpp>
#include <gs2/environment.hpp>
#include <gs2/script.hpp>

#include <expected>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

namespace og::gs2::testing {
    struct harness {
        environment env;
        dictionary_ptr self = std::make_shared<basic_dictionary>();
        script_ptr script;

        explicit harness(const std::string_view source) {
            std::istringstream is{std::string(source)};

            auto loaded = load_script(env, is);
            if (loaded) {
                script = *loaded;
                script->set_name("test.gs2");
            } else {
                error = loaded.error().message;
            }
        }

        std::string error;

        auto call(const std::string_view function_name, const values &args = {}) -> expected_value {
            if (!script) {
                return std::unexpected(gs2::error{
                    .source = error_source::parser,
                    .kind = error_kind::syntax_error,
                    .message = error,
                });
            }

            return script->call(function_name, self, args);
        }

        auto number(const std::string_view function_name, const values &args = {}) -> double {
            const auto result = call(function_name, args);

            return result ? to_number(*result) : 0.0;
        }

        auto text(const std::string_view function_name, const values &args = {}) -> std::string {
            const auto result = call(function_name, args);

            return result ? to_string(*result) : std::string{};
        }
    };

    inline auto evaluate(const std::string_view expression) -> double {
        auto script = harness(std::string("function main() { return ") + std::string(expression) + "; }");

        return script.number("main");
    }
}
