#include <catch2/catch_test_macros.hpp>

#include "script_harness.hpp"

using namespace std;
using namespace og::gs2;
using namespace og::gs2::testing;

TEST_CASE("bitwise operators", "[operators]") {
    REQUIRE(evaluate("12 & 10") == 8.0);
    REQUIRE(evaluate("12 | 10") == 14.0);
    REQUIRE(evaluate("12 xor 10") == 6.0);
    REQUIRE(evaluate("1 << 4") == 16.0);
    REQUIRE(evaluate("64 >> 3") == 8.0);
    REQUIRE(evaluate("~0") == -1.0);
}

TEST_CASE("bitwise operators truncate toward zero", "[operators]") {
    REQUIRE(evaluate("12.9 & 10.9") == 8.0);
    REQUIRE(evaluate("-3.7 | 0") == -3.0);
}

TEST_CASE("bitwise precedence sits below comparison", "[operators]") {
    REQUIRE(evaluate("1 | 2 == 2") == 1.0);
    REQUIRE(evaluate("(1 | 2) == 3") == 1.0);
}

TEST_CASE("shifts bind tighter than comparison and looser than arithmetic", "[operators]") {
    REQUIRE(evaluate("1 << 2 + 1") == 8.0);
    REQUIRE(evaluate("(1 << 2) + 1") == 5.0);
}

TEST_CASE("power uses ^ and its compound form", "[operators]") {
    REQUIRE(evaluate("2 ^ 10") == 1024.0);

    auto script = harness(R"(
        function main() {
            temp.value = 3;
            temp.value ^= 4;
            return temp.value;
        }
    )");

    REQUIRE(script.number("main") == 81.0);
}

TEST_CASE("compound bitwise assignment", "[operators]") {
    auto script = harness(R"(
        function main() {
            temp.flags = 12;
            temp.flags &= 10;
            temp.flags |= 1;
            temp.flags <<= 2;
            temp.flags >>= 1;
            return temp.flags;
        }
    )");

    REQUIRE(script.number("main") == 18.0);
}

TEST_CASE("multiply assignment multiplies", "[operators]") {
    auto script = harness(R"(
        function main() {
            temp.value = 6;
            temp.value *= 7;
            return temp.value;
        }
    )");

    REQUIRE(script.number("main") == 42.0);
}

TEST_CASE(":= is an alias for assignment", "[operators]") {
    auto script = harness(R"(
        function main() {
            temp.value := 5;
            return temp.value;
        }
    )");

    REQUIRE(script.number("main") == 5.0);
}

TEST_CASE("=< and => are aliases for <= and >=", "[operators]") {
    REQUIRE(evaluate("3 =< 3") == 1.0);
    REQUIRE(evaluate("4 =< 3") == 0.0);
    REQUIRE(evaluate("3 => 3") == 1.0);
    REQUIRE(evaluate("2 => 3") == 0.0);
}

TEST_CASE("in-range accepts every bracket combination", "[operators]") {
    REQUIRE(evaluate("5 in |1,5|") == 1.0);
    REQUIRE(evaluate("5 in <1,5>") == 0.0);
    REQUIRE(evaluate("1 in <1,5|") == 0.0);
    REQUIRE(evaluate("5 in <1,5|") == 1.0);
    REQUIRE(evaluate("1 in |1,5>") == 1.0);
    REQUIRE(evaluate("5 in |1,5>") == 0.0);
}

TEST_CASE("in-array still works alongside bitwise or", "[operators]") {
    REQUIRE(evaluate("3 in {1,2,3}") == 1.0);
    REQUIRE(evaluate("4 in {1,2,3}") == 0.0);
}

TEST_CASE("null equals nothing but emptiness", "[operators]") {
    REQUIRE(evaluate("NULL == 0") == 1.0);
    REQUIRE(evaluate("NULL == \"\"") == 1.0);
    REQUIRE(evaluate("NULL == \"thamhic-idle\"") == 0.0);
    REQUIRE(evaluate("NULL != \"thamhic-idle\"") == 1.0);
    REQUIRE(evaluate("\"guild\" == NULL") == 0.0);
    REQUIRE(evaluate("NULL == 3") == 0.0);
    REQUIRE(evaluate("NULL == {1,2}") == 0.0);
}

TEST_CASE("a comma joins expressions into one statement", "[operators]") {
    auto script = harness(R"(
        function pair() {
            temp.a = 50, temp.b = 60;

            return temp.a + temp.b;
        }

        function stillSeparates() {
            temp.list = { 1, 2, 3 };

            return temp.list.size();
        }
    )");

    REQUIRE(script.number("pair") == 110.0);
    REQUIRE(script.number("stillSeparates") == 3.0);
}

TEST_CASE("a leading @ names a variable, like makevar", "[operators]") {
    auto script = harness(R"(
        function reads() {
            temp.slot = "written";
            temp.name = "temp.slot";

            return @temp.name;
        }

        function writes() {
            temp.name = "temp.slot";
            @temp.name = "assigned";

            return temp.slot;
        }

        function stillConcatenates() {
            temp.a = "one";

            return temp.a @ "two";
        }
    )");

    REQUIRE(script.text("reads") == "written");
    REQUIRE(script.text("writes") == "assigned");
    REQUIRE(script.text("stillConcatenates") == "onetwo");
}

TEST_CASE("only a numeric string compares as a number", "[operators]") {
    auto script = harness(R"(
        function digits()   { return ("5" == 5); }
        function padded()   { return (" 5 " == 5); }
        function empty()    { return ("" == 0); }
        function words()    { return ("world" == 0); }
        function trailing() { return ("5 apples" == 5); }
        function numbers()  { return (5 == 5.0); }
    )");

    REQUIRE(script.number("digits") == 1.0);
    REQUIRE(script.number("padded") == 1.0);
    REQUIRE(script.number("empty") == 1.0);
    REQUIRE(script.number("words") == 0.0);
    REQUIRE(script.number("trailing") == 0.0);
    REQUIRE(script.number("numbers") == 1.0);
}
