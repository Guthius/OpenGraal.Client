#include <catch2/catch_test_macros.hpp>

#include <shared/text.hpp>

using namespace std;
using namespace og::shared;

TEST_CASE("trim removes surrounding whitespace", "[text]") {
    REQUIRE(trim("  hello  ") == "hello");
    REQUIRE(trim("\t\r\nhello\r\n") == "hello");
    REQUIRE(trim("   ").empty());
    REQUIRE(trim("").empty());
    REQUIRE(trim("a b") == "a b");
}

TEST_CASE("to_lower lowercases ascii", "[text]") {
    REQUIRE(to_lower("HeLLo.NW") == "hello.nw");
    REQUIRE(to_lower("").empty());
}

TEST_CASE("iequals compares case insensitively", "[text]") {
    REQUIRE(iequals("Level.NW", "level.nw"));
    REQUIRE_FALSE(iequals("level", "levels"));
}

TEST_CASE("split keeps empty fields", "[text]") {
    const auto parts = split("a,,b", ',');

    REQUIRE(parts.size() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1].empty());
    REQUIRE(parts[2] == "b");
}

TEST_CASE("split_whitespace collapses runs and drops empties", "[text]") {
    const auto parts = split_whitespace("  BOARD   0 1\t64 ");

    REQUIRE(parts.size() == 4);
    REQUIRE(parts[0] == "BOARD");
    REQUIRE(parts[3] == "64");
    REQUIRE(split_whitespace("   ").empty());
}

TEST_CASE("numeric conversions fall back on garbage", "[text]") {
    REQUIRE(to_int("42") == 42);
    REQUIRE(to_int("-7") == -7);
    REQUIRE(to_int("30.5") == 30);
    REQUIRE(to_int("", 9) == 9);
    REQUIRE(to_int("abc", 9) == 9);

    REQUIRE(to_float("30.5") == 30.5f);
    REQUIRE(to_float("abc", 1.5f) == 1.5f);

    REQUIRE(to_bool("true"));
    REQUIRE(to_bool("1"));
    REQUIRE_FALSE(to_bool("false"));
    REQUIRE(to_bool("", true));
}
