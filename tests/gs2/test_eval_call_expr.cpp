#include <catch2/catch_test_macros.hpp>

#include "utils.hpp"

#include <gs2/value.hpp>

using namespace std;
using namespace og::gs2;

namespace {
    auto evaluate(const ast::expr &expr) -> expected_value {
        auto self = make_shared<basic_dictionary>();

        auto test_environment = environment();
        auto text_context = context(test_environment, self);

        return eval(text_context, expr);
    }
}

TEST_CASE("Call expressions evaluate as error when callee is not callable", "[eval]") {
    SECTION("Identifier that does not resolve to a callable") {
        auto expr = make_expr(ast::call_expr{
            .callee = make_expr(ast::identifier_expr{
                .name = "test_func",
            }),
        });

        auto result = evaluate(expr);

        REQUIRE(!result.has_value());
    }

    SECTION("Number literal") {
        auto expr = make_expr(ast::call_expr{
            .callee = make_expr(ast::number_expr{
                .value = 1.0,
            }),
        });

        auto result = evaluate(expr);

        REQUIRE(!result.has_value());
    }

    SECTION("String literal") {
        auto expr = make_expr(ast::call_expr{
            .callee = make_expr(ast::string_expr{
                .value = "test_func",
            }),
        });

        auto result = evaluate(expr);

        REQUIRE(!result.has_value());
    }
}

TEST_CASE("Evaluating call expressions invokes callables with arguments") {
    static constexpr auto test_value = 10.25;
    static constexpr auto argument_a = 123.45;
    static constexpr string argument_b = "Test str";

    ast::expr_list expr_args;
    expr_args.push_back(make_expr(ast::number_expr{.value = argument_a}));
    expr_args.push_back(make_expr(ast::string_expr{.value = argument_b}));

    auto expr = make_expr(ast::call_expr{
        .callee = make_expr(ast::identifier_expr{
            .name = "test_func",
        }),
        .args = std::move(expr_args),
    });

    auto self = make_shared<basic_dictionary>();

    auto test_environment = environment();
    auto text_context = context(test_environment, self);

    test_environment.register_function(
        "test_func",
        [](auto &env, const auto &args) -> expected_value {
            REQUIRE((args.size() == 2));
            REQUIRE(has_value(args[0], argument_a));
            REQUIRE(has_value(args[1], argument_b));

            return value{test_value};
        });

    auto result = eval(text_context, expr);

    REQUIRE(has_value(result, test_value));
}
