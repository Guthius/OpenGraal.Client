#include <catch2/catch_test_macros.hpp>

#include "script_harness.hpp"

#include <gs2/context.hpp>

using namespace std;
using namespace og::gs2;
using namespace og::gs2::testing;

TEST_CASE("temp is a real call frame, not a field on the object", "[scoping]") {
    auto script = harness(R"(
        function main() {
            temp.value = 1;
            return temp.value;
        }
    )");

    REQUIRE(script.number("main") == 1.0);
    REQUIRE_FALSE(script.self->contains("temp"));
}

TEST_CASE("temp does not leak between nested calls", "[scoping]") {
    auto script = harness(R"(
        function inner() {
            temp.value = 2;
            return temp.value;
        }

        function main() {
            temp.value = 1;
            temp.inner = inner();
            return temp.value;
        }
    )");

    REQUIRE(script.number("main") == 1.0);
}

TEST_CASE("recursion keeps a separate frame per call", "[scoping]") {
    auto script = harness(R"(
        function fact(n) {
            if (temp.n <= 1) return 1;
            return temp.n * fact(temp.n - 1);
        }

        function main() {
            return fact(6);
        }
    )");

    REQUIRE(script.number("main") == 720.0);
}

TEST_CASE("runaway recursion is caught instead of crashing", "[scoping]") {
    auto script = harness(R"(
        function forever() {
            return forever();
        }
    )");

    const auto result = script.call("forever");

    REQUIRE_FALSE(result);
    REQUIRE(result.error().message.contains("call stack"));
}

TEST_CASE("this and thiso address the object, bare names do not", "[scoping]") {
    auto script = harness(R"(
        function main() {
            this.stored = 7;
            local = 9;
            return this.stored;
        }
    )");

    REQUIRE(script.number("main") == 7.0);
    REQUIRE(to_number(script.self->get("stored")) == 7.0);
    REQUIRE_FALSE(script.self->contains("local"));
}

TEST_CASE("thiso is an alias for this", "[scoping]") {
    auto script = harness(R"(
        function main() {
            this.value = 3;
            return thiso.value;
        }
    )");

    REQUIRE(script.number("main") == 3.0);
}

TEST_CASE("script locals persist across calls", "[scoping]") {
    auto script = harness(R"(
        function bump() {
            counter = counter + 1;
            return counter;
        }
    )");

    REQUIRE(script.number("bump") == 1.0);
    REQUIRE(script.number("bump") == 2.0);
    REQUIRE(script.number("bump") == 3.0);
}

TEST_CASE("host scopes resolve by name", "[scoping]") {
    auto env = environment();
    auto self = make_shared<basic_dictionary>();
    auto level = make_shared<basic_dictionary>();

    level->put("difficulty", value{4.0});

    auto ctx = context(env, self);

    REQUIRE_FALSE(ctx.find_scope("level"));

    ctx.bind_scope("level", level);

    REQUIRE(ctx.find_scope("level") == level);

    const auto resolved = ctx.get("level");
    const auto *dictionary = get_if<dictionary_ptr>(&resolved);

    REQUIRE(dictionary != nullptr);
    REQUIRE(to_number((*dictionary)->get("difficulty")) == 4.0);
}

TEST_CASE("a $ name resolves through nested scope resolution", "[scoping]") {
    auto script = harness(R"(
        function main() {
            return $pref::graal::defaultfontsize / 24;
        }
    )");

    auto prefs = make_shared<basic_dictionary>();
    auto graal = make_shared<basic_dictionary>();

    graal->put("defaultfontsize", value{24.0});
    prefs->put("graal", value{graal});

    script.env.put("$pref", value{prefs});

    REQUIRE(script.number("main") == 1.0);
}

TEST_CASE("a lone $ is not a name", "[scoping]") {
    auto script = harness(R"(
        function main() {
            return $ + 1;
        }
    )");

    REQUIRE_FALSE(script.script);
}

TEST_CASE("resolving through something that is not an object is an error", "[scoping]") {
    auto script = harness(R"(
        function main() {
            return $pref::graal::defaultfontsize;
        }
    )");

    const auto result = script.call("main");

    REQUIRE_FALSE(result);
    REQUIRE(result.error().message.contains("unknown scope"));
}

TEST_CASE("assigning to a reserved scope name is an error", "[scoping]") {
    auto script = harness(R"(
        function main() {
            temp = 1;
        }
    )");

    const auto result = script.call("main");

    REQUIRE_FALSE(result);
    REQUIRE(result.error().message.contains("not assignable"));
}

TEST_CASE("params exposes the raw argument list", "[scoping]") {
    auto script = harness(R"(
        function main() {
            return params[0] + params[1];
        }
    )");

    REQUIRE(script.number("main", {value{10.0}, value{5.0}}) == 15.0);
}

TEST_CASE("a declared parameter answers to its bare name", "[scoping]") {
    auto script = harness(R"(
        function greet(who) {
            return "hello" SPC who;
        }

        function main() {
            return greet("world");
        }
    )");

    REQUIRE(script.text("main") == "hello world");
}

TEST_CASE("assigning to a bare parameter name writes the parameter, not a local", "[scoping]") {
    auto script = harness(R"(
        function twice(n) {
            n = n * 2;

            return temp.n;
        }

        function main() {
            return twice(21);
        }
    )");

    REQUIRE(script.number("main") == 42.0);
    REQUIRE_FALSE(script.self->contains("n"));
}

TEST_CASE("a parameter is only bare-addressable inside its own call", "[scoping]") {
    auto script = harness(R"(
        function inner() {
            return who;
        }

        function outer(who) {
            return inner();
        }

        function main() {
            return outer("visible") @ "!";
        }
    )");

    REQUIRE(script.text("main") == "!");
}

TEST_CASE("named parameters land in temp", "[scoping]") {
    auto script = harness(R"(
        function main(first, second) {
            return temp.first @ "-" @ temp.second;
        }
    )");

    REQUIRE(script.text("main", {value{string("a")}, value{string("b")}}) == "a-b");
}

TEST_CASE("errors carry the script and function they came from", "[scoping]") {
    auto script = harness(R"(
        function main() {
            temp = 1;
        }
    )");

    const auto result = script.call("main");

    REQUIRE_FALSE(result);
    REQUIRE(result.error().source_name == "test.gs2");
    REQUIRE(result.error().function_name == "main");
}
