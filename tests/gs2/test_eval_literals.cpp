#include <catch2/catch_test_macros.hpp>

#include "script_harness.hpp"
#include "utils.hpp"

using namespace std;
using namespace og::gs2;
using namespace og::gs2::testing;

namespace {
    auto evaluate(const ast::expr &expr) -> expected_value {
        auto self = make_shared<basic_dictionary>();

        auto test_environment = environment();
        auto text_context = context(test_environment, self);

        return eval(text_context, expr);
    }
}

TEST_CASE("Number expression evaluates to correct value", "[eval]") {
    static constexpr auto test_value = 12.5;

    auto expr = make_expr(ast::number_expr{
        .value = test_value,
    });

    REQUIRE(has_value(evaluate(expr), test_value));
}

TEST_CASE("String expression evaluates to correct value", "[eval]") {
    static constexpr string test_value = "Hello World";

    auto expr = make_expr(ast::string_expr{
        .value = test_value,
    });

    REQUIRE(has_value(evaluate(expr), test_value));
}

TEST_CASE("True expression evaluates to correct value", "[eval]") {
    auto expr = make_expr(ast::boolean_expr{
        .value = true,
    });

    REQUIRE(has_value(evaluate(expr), 1.0));
}

TEST_CASE("False expression evaluates to correct value", "[eval]") {
    auto expr = make_expr(ast::boolean_expr{
        .value = false,
    });

    REQUIRE(has_value(evaluate(expr), 0.0));
}

TEST_CASE("a string literal may be single-quoted", "[eval]") {
    auto script = harness(R"(
        function main() {
            return 'back' @ "tick";
        }
    )");

    REQUIRE(script.text("main") == "backtick");
}

TEST_CASE("tab and quote escapes are accepted in both literal forms", "[eval]") {
    auto script = harness(R"(
        function main() {
            return "a\tb" @ 'c\td' @ '\'' @ "\"";
        }
    )");

    REQUIRE(script.text("main") == "a\tbc\td'\"");
}
