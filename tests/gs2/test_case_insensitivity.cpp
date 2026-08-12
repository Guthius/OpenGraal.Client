#include <catch2/catch_test_macros.hpp>

#include "script_harness.hpp"

#include <gs2/prototype.hpp>

using namespace std;
using namespace og::gs2;
using namespace og::gs2::testing;

namespace {
    struct gadget {
        string text;
    };

    auto gadget_prototype() -> prototype_ptr {
        return prototype_builder<gadget>("Gadget")
            .constructor()
            .property(
                "text",
                [](const gadget *self) { return self->text; },
                [](gadget *self, const string &value) { self->text = value; })
            .function("settext", [](gadget *self, const values &args) -> expected_value {
                self->text = args.empty() ? string{} : to_string(args[0]);

                return value{};
            })
            .build();
    }
}

TEST_CASE("a script function is callable in any case", "[case]") {
    auto script = harness(R"(
        function onActionServerside() {
            return 7;
        }
    )");

    REQUIRE(script.number("onactionserverside") == 7.0);
    REQUIRE(script.number("ONACTIONSERVERSIDE") == 7.0);
    REQUIRE(script.script->has_function("onActionserverside"));
}

TEST_CASE("a script function declared in an unusual case still dispatches", "[case]") {
    auto script = harness(R"(
        function onPlayerchats() {
            return 3;
        }
    )");

    REQUIRE(script.number("onPlayerChats") == 3.0);
}

TEST_CASE("public enforcement is case-insensitive", "[case]") {
    auto script = harness(R"(
        public function onActionServerside() {
            return 1;
        }
    )");

    REQUIRE(script.script->is_public("onactionserverside"));
    REQUIRE(to_number(*script.script->call_public("onactionserverside", script.self)) == 1.0);
}

TEST_CASE("a script calls its own function in another case", "[case]") {
    auto script = harness(R"(
        function helper() {
            return 4;
        }

        function main() {
            return Helper() + HELPER();
        }
    )");

    REQUIRE(script.number("main") == 8.0);
}

TEST_CASE("standard functions resolve in any case", "[case]") {
    REQUIRE(evaluate("Int(3.7)") == 3.0);
    REQUIRE(evaluate("MAX(2, 9)") == 9.0);
    REQUIRE(evaluate("getAngle(1, 0)") == 0.0);
}

TEST_CASE("value methods resolve in any case", "[case]") {
    auto script = harness(R"(
        function main() {
            temp.text = "  hello  ";
            return temp.text.Trim().Length();
        }
    )");

    REQUIRE(script.number("main") == 5.0);

    auto array_script = harness(R"(
        function main() {
            temp.items = {1, 2};
            temp.items.Add(3);
            return temp.items.Size();
        }
    )");

    REQUIRE(array_script.number("main") == 3.0);
}

TEST_CASE("prototype properties and methods resolve in any case", "[case]") {
    auto script = harness(R"(
        function main() {
            temp.gadget = new Gadget();
            temp.gadget.setText("done");
            return temp.gadget.Text;
        }
    )");

    script.env.register_type(gadget_prototype());

    REQUIRE(script.text("main") == "done");
}

TEST_CASE("joined class functions resolve in any case", "[case]") {
    auto script = harness(R"(
        function main() {
            join("helpers");
            return Greeting();
        }
    )");

    script.env.set_class_resolver([](const string_view name) -> optional<string> {
        if (name == "helpers") {
            return string(R"(function greeting() { return 11; })");
        }

        return nullopt;
    });

    REQUIRE(script.number("main") == 11.0);
}
