#include <catch2/catch_test_macros.hpp>

#include "script_harness.hpp"

#include <algorithm>
#include <map>

using namespace std;
using namespace og::gs2;
using namespace og::gs2::testing;

namespace {
    auto class_library(harness &script, map<string, string> classes) {
        script.env.set_class_resolver([classes = std::move(classes)](const string_view name) -> optional<string> {
            const auto match = classes.find(string(name));
            if (match == classes.end()) {
                return nullopt;
            }

            return match->second;
        });
    }
}

TEST_CASE("join pulls in a class script", "[join]") {
    auto script = harness(R"(
        function onCreated() {
            join("helpers");
        }

        function main() {
            return greet();
        }
    )");

    class_library(script, {
                              {"helpers", R"(function greet() { return "hello"; })"}
    });

    REQUIRE(script.call("onCreated"));
    REQUIRE(script.text("main") == "hello");
}

TEST_CASE("a joined class overrides an existing function", "[join]") {
    auto script = harness(R"(
        function greet() {
            return "original";
        }

        function onCreated() {
            join("override");
        }
    )");

    class_library(script, {
                              {"override", R"(function greet() { return "replaced"; })"}
    });

    REQUIRE(script.text("greet") == "original");
    REQUIRE(script.call("onCreated"));
    REQUIRE(script.text("greet") == "replaced");
}

TEST_CASE("the most recently joined class wins", "[join]") {
    auto script = harness(R"(
        function onCreated() {
            join("first");
            join("second");
        }
    )");

    class_library(script, {
                              {"first",  R"(function which() { return "first"; })" },
                              {"second", R"(function which() { return "second"; })"},
    });

    REQUIRE(script.call("onCreated"));
    REQUIRE(script.text("which") == "second");
}

TEST_CASE("joining the same class twice is a no-op", "[join]") {
    auto script = harness(R"(
        function onCreated() {
            temp.first = join("helpers");
            temp.second = join("helpers");
            return temp.first @ ":" @ temp.second;
        }
    )");

    class_library(script, {
                              {"helpers", R"(function greet() { return "hi"; })"}
    });

    REQUIRE(script.text("onCreated") == "1:0");
}

TEST_CASE("joining an unknown class reports the name", "[join]") {
    auto script = harness(R"(
        function onCreated() {
            join("nosuchclass");
        }
    )");

    class_library(script, {});

    const auto result = script.call("onCreated");

    REQUIRE_FALSE(result);
    REQUIRE(result.error().message.contains("nosuchclass"));
}

TEST_CASE("has_function sees through joined classes", "[join]") {
    auto script = harness(R"(
        function onCreated() {
            join("helpers");
        }
    )");

    class_library(script, {
                              {"helpers", R"(function greet() { return 1; })"}
    });

    REQUIRE_FALSE(script.script->has_function("greet"));
    REQUIRE(script.call("onCreated"));
    REQUIRE(script.script->has_function("greet"));
    REQUIRE(script.script->has_joined("helpers"));
}

TEST_CASE("the clientside marker splits a script in two", "[join]") {
    const auto halves = split_script(
        "function onCreated() {\n"
        "  this.x = 1;\n"
        "}\n"
        "//#CLIENTSIDE\n"
        "function onKeyPressed() {\n"
        "  this.y = 2;\n"
        "}\n");

    REQUIRE(halves.server.contains("onCreated"));
    REQUIRE_FALSE(halves.server.contains("onKeyPressed"));

    REQUIRE(halves.client.contains("onKeyPressed"));
    REQUIRE_FALSE(halves.client.contains("onCreated"));
}

TEST_CASE("both halves keep the original line numbering", "[join]") {
    const auto halves = split_script(
        "line1\n"
        "//#CLIENTSIDE\n"
        "line3\n");

    const auto count_lines = [](const string &text) {
        return count(text.begin(), text.end(), '\n');
    };

    REQUIRE(count_lines(halves.server) == 3);
    REQUIRE(count_lines(halves.client) == 3);
    REQUIRE(halves.client.starts_with("\n\nline3"));
}

TEST_CASE("a script without the marker is all server side", "[join]") {
    const auto halves = split_script("function onCreated() {}\n");

    REQUIRE(halves.server.contains("onCreated"));
    REQUIRE(halves.client.find_first_not_of('\n') == string::npos);
}
