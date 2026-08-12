#include <catch2/catch_test_macros.hpp>

#include "utils.hpp"

#include <gs2/value.hpp>

using namespace std;
using namespace og::gs2;

TEST_CASE("Assign to member expression updates member field") {
    static constexpr auto test_value = 25.88;

    auto expr = make_expr(ast::assign_expr{
        .op = token_kind::op_assign,
        .target = make_expr(ast::member_expr{
            .object = make_expr(ast::identifier_expr{
                .name = "player",
            }),
            .member = make_expr(ast::identifier_expr{
                .name = "x"}),
        }),
        .value = make_expr(ast::number_expr{
            .value = test_value,
        }),
    });

    auto self = make_shared<basic_dictionary>();
    auto player = make_shared<basic_dictionary>();

    auto test_environment = environment();
    auto text_context = context(test_environment, self);

    test_environment.put("player", player);

    auto result = eval(text_context, expr);

    REQUIRE(player->contains("x"));
    REQUIRE(has_value(result, test_value));
}
