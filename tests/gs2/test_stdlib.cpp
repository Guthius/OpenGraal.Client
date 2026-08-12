#include <catch2/catch_test_macros.hpp>

#include <optional>

#include "script_harness.hpp"

#include <gs2/builtins.hpp>

#include <cmath>

using namespace std;
using namespace og::gs2;
using namespace og::gs2::testing;

TEST_CASE("string methods", "[stdlib]") {
    auto script = harness(R"(
        function length()    { return "hello".length(); }
        function trimmed()   { return ("  hi  ").trim(); }
        function starts()    { return "swimidle".starts("swim"); }
        function ends()      { return "swimidle".ends("idle"); }
        function pos()       { return "abcdef".pos("cd"); }
        function missing()   { return "abcdef".pos("zz"); }
        function charat()    { return "abc".charat(1); }
        function substr()    { return "abcdef".substring(2); }
        function substr2()   { return "abcdef".substring(2, 3); }
        function substrall() { return "abcdef".substring(4, -1); }
    )");

    REQUIRE(script.number("length") == 5.0);
    REQUIRE(script.text("trimmed") == "hi");
    REQUIRE(script.number("starts") == 1.0);
    REQUIRE(script.number("ends") == 1.0);
    REQUIRE(script.number("pos") == 2.0);
    REQUIRE(script.number("missing") == -1.0);
    REQUIRE(script.text("charat") == "b");
    REQUIRE(script.text("substr") == "cdef");
    REQUIRE(script.text("substr2") == "cde");
    REQUIRE(script.text("substrall") == "ef");
}

TEST_CASE("tokenize splits on the given delimiters", "[stdlib]") {
    auto script = harness(R"(
        function count() {
            temp.parts = ("I am happy!").tokenize(" ");
            return temp.parts.size();
        }

        function second() {
            temp.parts = ("a,b,c").tokenize(",");
            return temp.parts[1];
        }
    )");

    REQUIRE(script.number("count") == 3.0);
    REQUIRE(script.text("second") == "b");
}

TEST_CASE("array methods", "[stdlib]") {
    auto script = harness(R"(
        function main() {
            temp.items = {1, 2, 3};
            temp.items.add(4);
            temp.items.insert(0, 0);
            temp.items.delete(1);
            temp.items.replace(0, 9);
            return temp.items.size() @ ":" @ temp.items[0] @ ":" @ temp.items.index(4);
        }

        function removal() {
            temp.items = {"a", "b", "c"};
            temp.items.remove("b");
            return temp.items.size() @ temp.items[1];
        }

        function cleared() {
            temp.items = {1, 2};
            temp.items.clear();
            return temp.items.size();
        }

        function sliced() {
            temp.items = {1, 2, 3, 4, 5};
            temp.slice = temp.items.subarray(1, 3);
            return temp.slice.size() @ ":" @ temp.slice[0];
        }

        function joined() {
            temp.items = {1, 2};
            temp.items.addarray({3, 4});
            return temp.items.size();
        }
    )");

    REQUIRE(script.text("main") == "4:9:3");
    REQUIRE(script.text("removal") == "2c");
    REQUIRE(script.number("cleared") == 0.0);
    REQUIRE(script.text("sliced") == "3:2");
    REQUIRE(script.number("joined") == 4.0);
}

TEST_CASE("type reports the value kind", "[stdlib]") {
    auto script = harness(R"(
        function number() { return (1).type(); }
        function text()   { return "a".type(); }
        function list()   { return {1,2}.type(); }
    )");

    REQUIRE(script.number("number") == 0.0);
    REQUIRE(script.number("text") == 1.0);
    REQUIRE(script.number("list") == 3.0);
}

TEST_CASE("standard math functions", "[stdlib]") {
    REQUIRE(evaluate("int(3.7)") == 3.0);
    REQUIRE(evaluate("int(-3.7)") == -3.0);
    REQUIRE(evaluate("abs(-4)") == 4.0);
    REQUIRE(evaluate("min(3, 7)") == 3.0);
    REQUIRE(evaluate("max(3, 7)") == 7.0);
    REQUIRE(evaluate("log(2, 8)") == 3.0);
    REQUIRE(evaluate("exp(0)") == 1.0);
    REQUIRE(abs(evaluate("sin(0)")) < 1e-9);
    REQUIRE(abs(evaluate("cos(0)") - 1.0) < 1e-9);
}

TEST_CASE("random stays inside its range", "[stdlib]") {
    for (auto i = 0; i < 32; ++i) {
        const auto rolled = evaluate("random(2, 5)");

        REQUIRE(rolled >= 2.0);
        REQUIRE(rolled <= 5.0);
    }
}

TEST_CASE("direction helpers match the movement convention", "[stdlib]") {
    REQUIRE(evaluate("vecx(0)") == 0.0);
    REQUIRE(evaluate("vecy(0)") == -1.0);
    REQUIRE(evaluate("vecx(1)") == -1.0);
    REQUIRE(evaluate("vecy(2)") == 1.0);
    REQUIRE(evaluate("vecx(3)") == 1.0);

    REQUIRE(evaluate("getdir(5, 0)") == 3.0);
    REQUIRE(evaluate("getdir(-5, 0)") == 1.0);
    REQUIRE(evaluate("getdir(0, -5)") == 0.0);
    REQUIRE(evaluate("getdir(0, 5)") == 2.0);
}

TEST_CASE("moving along getangle advances in the direction asked for", "[stdlib]") {
    auto script = harness(R"(
        function step(direction) {
            temp.angle = getangle(vecx(temp.direction), vecy(temp.direction));
            temp.x = cos(temp.angle);
            temp.y = -sin(temp.angle);
            return int(temp.x * 100) @ ":" @ int(temp.y * 100);
        }
    )");

    REQUIRE(script.text("step", {value{2.0}}) == "0:100");
    REQUIRE(script.text("step", {value{0.0}}) == "0:-100");
    REQUIRE(script.text("step", {value{3.0}}) == "100:0");
    REQUIRE(script.text("step", {value{1.0}}) == "-100:0");
}

TEST_CASE("format substitutes printf style placeholders", "[stdlib]") {
    auto script = harness(R"GS2(
        function main() {
            return format("%s has %d hearts (%d%%)", "player", 3, 50);
        }
    )GS2");

    REQUIRE(script.text("main") == "player has 3 hearts (50%)");
}

TEST_CASE("lowercase and uppercase", "[stdlib]") {
    auto script = harness(R"(
        function lower() { return lowercase("MiXeD"); }
        function upper() { return uppercase("MiXeD"); }
    )");

    REQUIRE(script.text("lower") == "mixed");
    REQUIRE(script.text("upper") == "MIXED");
}

TEST_CASE("makevar resolves a computed name through the scope chain", "[stdlib]") {
    auto script = harness(R"(
        function main() {
            this.slot_2 = "found";
            temp.index = 2;
            return makevar("this.slot_" @ temp.index);
        }
    )");

    REQUIRE(script.text("main") == "found");
}

TEST_CASE("makevar on a missing path yields nothing", "[stdlib]") {
    auto script = harness(R"(
        function main() {
            return makevar("this.nope.deeper");
        }
    )");

    REQUIRE(script.text("main").empty());
}

TEST_CASE("a variable becomes an object when used as one", "[stdlib]") {
    auto script = harness(R"(
        function main() {
            temp.positions.gui_x = 12;
            return temp.positions.gui_x;
        }
    )");

    REQUIRE(script.number("main") == 12.0);
}

TEST_CASE("reading through a missing member does not create it", "[stdlib]") {
    auto script = harness(R"(
        function main() {
            temp.probe = this.missing.deeper;
            return this.missing == null;
        }
    )");

    REQUIRE(script.number("main") == 1.0);
}

TEST_CASE("loadvars reads a vars file into the object", "[stdlib]") {
    auto script = harness(R"(
        function main() {
            temp.positions.loadvars("guipositions.txt");
            return temp.positions.gui_status_x @ "," @ temp.positions.gui_status_y;
        }
    )");

    script.env.set_file_resolver([](const string_view name) -> optional<string> {
        if (name == "guipositions.txt") {
            return string("gui_status_x=40\ngui_status_y = 55 \n\nbroken line\n");
        }

        return nullopt;
    });

    REQUIRE(script.text("main") == "40,55");
}

TEST_CASE("loadvars on a file the host cannot supply is a no-op", "[stdlib]") {
    auto script = harness(R"(
        function main() {
            return temp.positions.loadvars("nothing.txt");
        }
    )");

    REQUIRE(script.number("main") == 0.0);
}

TEST_CASE("savevars writes the object back as a vars file", "[stdlib]") {
    auto script = harness(R"(
        function main() {
            temp.positions.gui_status_y = 55;
            temp.positions.gui_status_x = 40;
            temp.positions.title = "a name";
            return temp.positions.savevars("out.txt", 0);
        }
    )");

    string written;
    script.env.set_file_writer([&written](const string_view, const string_view contents) {
        written = contents;

        return true;
    });

    REQUIRE(script.number("main") == 1.0);
    REQUIRE(written == "gui_status_x=40\ngui_status_y=55\ntitle=a name\n");
}

TEST_CASE("savevars writes a nested object as dotted keys", "[stdlib]") {
    auto script = harness(R"(
        function main() {
            temp.account.item.realname = "falchion";
            temp.account.item.displayname = "Pirate Falchion";
            temp.account.level = 3;
            temp.account.savevars("out.txt", 0);
        }
    )");

    string written;
    script.env.set_file_writer([&written](const string_view, const string_view contents) {
        written = contents;

        return true;
    });

    REQUIRE(script.call("main"));
    REQUIRE(written == "item.displayname=Pirate Falchion\nitem.realname=falchion\nlevel=3\n");
}

TEST_CASE("savevars round-trips through loadvars", "[stdlib]") {
    auto script = harness(R"(
        function save() {
            temp.out.x = 12;
            temp.out.label = "hi";
            temp.out.savevars("round.txt", 0);
        }

        function load() {
            temp.back.loadvars("round.txt");
            return temp.back.x @ ":" @ temp.back.label;
        }
    )");

    auto stored = string{};

    script.env.set_file_writer([&stored](const string_view, const string_view contents) {
        stored = contents;

        return true;
    });
    script.env.set_file_resolver([&stored](const string_view) -> optional<string> {
        return stored.empty() ? nullopt : optional(stored);
    });

    REQUIRE(script.call("save"));
    REQUIRE(script.text("load") == "12:hi");
}

TEST_CASE("savevars with no host writer reports failure", "[stdlib]") {
    auto script = harness(R"(
        function main() {
            temp.out.x = 1;
            return temp.out.savevars("nowhere.txt", 0);
        }
    )");

    REQUIRE(script.number("main") == 0.0);
}

TEST_CASE("getdynamicvarnames lists what a script set", "[stdlib]") {
    auto script = harness(R"(
        function main() {
            temp.bag.apple = 1;
            temp.bag.pear = 2;
            temp.names = temp.bag.getdynamicvarnames();
            temp.names.add("");
            return temp.names.size();
        }
    )");

    REQUIRE(script.number("main") == 3.0);
}

TEST_CASE("a null variable becomes what the call needs", "[stdlib]") {
    auto script = harness(R"(
        function list() {
            temp.names.add("a");
            temp.names.add("b");

            return temp.names.size();
        }

        function afternull() {
            temp.cleared = NULL;
            temp.cleared.add("only");

            return temp.cleared[0];
        }

        function object() {
            temp.record.field = "set";

            return temp.record.field;
        }
    )");

    REQUIRE(script.number("list") == 2.0);
    REQUIRE(script.text("afternull") == "only");
    REQUIRE(script.text("object") == "set");
}

TEST_CASE("makevar names a variable, and can be assigned through", "[stdlib]") {
    auto script = harness(R"(
        function readwrite() {
            makevar("temp.slot_1") = "written";

            return temp.slot_1;
        }

        function nested() {
            temp.record.name = "carol";

            return makevar("temp.record.name");
        }

        function builds() {
            makevar("temp.fresh").add("one");
            makevar("temp.fresh").add("two");

            return temp.fresh.size();
        }

        function unsets() {
            temp.gone = "here";
            unset("temp.gone");

            return temp.gone;
        }
    )");

    REQUIRE(script.text("readwrite") == "written");
    REQUIRE(script.text("nested") == "carol");
    REQUIRE(script.number("builds") == 2.0);
    REQUIRE(script.text("unsets").empty());
}

TEST_CASE("loadvarsfromarray replaces what the object held", "[stdlib]") {
    auto script = harness(R"(
        function pairs() {
            temp.lines.add("name=carol");
            temp.lines.add("level=5");
            temp.record.loadvarsfromarray(temp.lines);

            return temp.record.name @ "/" @ temp.record.level;
        }

        function replaces() {
            temp.first.add("name=carol");
            temp.record.loadvarsfromarray(temp.first);

            temp.second.add("level=5");
            temp.record.loadvarsfromarray(temp.second);

            return "[" @ temp.record.name @ "]" @ temp.record.level;
        }

        function commas() {
            temp.record.loadvarsfromarray("name=carol,level=5");

            return temp.record.name @ "/" @ temp.record.level;
        }
    )");

    REQUIRE(script.text("pairs") == "carol/5");
    REQUIRE(script.text("replaces") == "[]5");
    REQUIRE(script.text("commas") == "carol/5");
}

TEST_CASE("a list reads as its elements, comma-separated", "[stdlib]") {
    auto script = harness(R"(
        function joined() {
            temp.names.add("a");
            temp.names.add("b");
            temp.names.add("c");

            return "" @ temp.names;
        }

        function empty() {
            temp.nothing.add("only");
            temp.nothing.delete(0);

            return "[" @ temp.nothing @ "]";
        }
    )");

    REQUIRE(script.text("joined") == "a,b,c");
    REQUIRE(script.text("empty") == "[]");
}

TEST_CASE("loadvars nests dotted keys, and savevars puts them back", "[stdlib]") {
    auto script = harness(R"(
        function read() {
            temp.guild.loadvars("guild.txt");

            return temp.guild.member_ApothiX.account @ "/" @ temp.guild.member_ApothiX.rank;
        }

        function names() {
            temp.guild.loadvars("guild.txt");

            return temp.guild.getdynamicvarnames().size();
        }

        function roundtrip() {
            temp.guild.loadvars("guild.txt");
            temp.guild.savevars("out.txt", 0);

            return 1;
        }
    )");

    const auto source = string(
        "leader=ApothiX\n"
        "member_ApothiX.account=ApothiX\n"
        "member_ApothiX.rank=Leader\n");

    string written;
    script.env.set_file_resolver([&source](const string_view) -> optional<string> { return source; });
    script.env.set_file_writer([&written](const string_view, const string_view contents) {
        written = contents;

        return true;
    });

    REQUIRE(script.text("read") == "ApothiX/Leader");
    REQUIRE(script.number("names") == 2.0);
    REQUIRE(script.call("roundtrip"));
    REQUIRE(written == source);
}
