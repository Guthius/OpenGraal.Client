#include <catch2/catch_test_macros.hpp>

#include "script_harness.hpp"

#include <gs2/prototype.hpp>

using namespace std;
using namespace og::gs2;
using namespace og::gs2::testing;

namespace {
    struct widget {
        double width = 0.0;
        double height = 0.0;
        string text;
    };

    auto widget_prototype() -> prototype_ptr {
        return prototype_builder<widget>("Widget")
            .constructor()
            .property(
                "width",
                [](const widget *self) { return self->width; },
                [](widget *self, const double value) { self->width = value; })
            .property(
                "height",
                [](const widget *self) { return self->height; },
                [](widget *self, const double value) { self->height = value; })
            .property(
                "text",
                [](const widget *self) { return self->text; },
                [](widget *self, const string &value) { self->text = value; })
            .function("area", [](const widget *self, const values &) -> expected_value {
                return value{self->width * self->height};
            })
            .build();
    }
}

TEST_CASE("new constructs a registered type", "[objects]") {
    auto script = harness(R"(
        function main() {
            temp.widget = new Widget();
            temp.widget.width = 4;
            temp.widget.height = 5;
            return temp.widget.area();
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 20.0);
}

TEST_CASE("a new body assigns onto the new object", "[objects]") {
    auto script = harness(R"(
        function main() {
            temp.widget = new Widget() {
                width = 3;
                height = 6;
            };

            return temp.widget.area();
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 18.0);
}

TEST_CASE("a new body can set fields the prototype does not declare", "[objects]") {
    auto script = harness(R"(
        function main() {
            temp.widget = new Widget() {
                width = 2;
                height = 2;
                tag = "custom";
            };

            return temp.widget.tag;
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.text("main") == "custom");
}

TEST_CASE("new on an unknown type reports the type name", "[objects]") {
    auto script = harness(R"(
        function main() {
            return new NoSuchThing();
        }
    )");

    const auto result = script.call("main");

    REQUIRE_FALSE(result);
    REQUIRE(result.error().message.contains("NoSuchThing"));
}

TEST_CASE("new [count] builds an array", "[objects]") {
    auto script = harness(R"(
        function main() {
            temp.items = new[4];
            temp.items[3] = 9;
            return temp.items[3];
        }
    )");

    REQUIRE(script.number("main") == 9.0);
}

TEST_CASE("with retargets this for the block", "[objects]") {
    auto script = harness(R"(
        function main() {
            temp.widget = new Widget();

            with (temp.widget) {
                this.width = 7;
                this.height = 2;
            }

            return temp.widget.area();
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 14.0);
}

TEST_CASE("with restores the previous this afterwards", "[objects]") {
    auto script = harness(R"(
        function main() {
            this.marker = 1;
            temp.widget = new Widget();

            with (temp.widget) {
                this.width = 3;
            }

            return this.marker;
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 1.0);
}

TEST_CASE("with leaves script locals alone", "[objects]") {
    auto script = harness(R"(
        function main() {
            counter = 5;
            temp.widget = new Widget();

            with (temp.widget) {
                counter = counter + 1;
            }

            return counter;
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 6.0);
}

TEST_CASE("thiso stays on the script object inside with", "[objects]") {
    auto script = harness(R"(
        function main() {
            this.marker = 5;
            temp.widget = new Widget();

            with (temp.widget) {
                this.width = 3;
                thiso.marker = thiso.marker + 1;
            }

            return this.marker;
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 6.0);
}

TEST_CASE("thiso stays on the script object inside a new body", "[objects]") {
    auto script = harness(R"(
        function main() {
            this.marker = 2;

            temp.widget = new Widget() {
                width = 4;
                thiso.marker = 9;
            };

            return this.marker;
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 9.0);
}

TEST_CASE("a bare name in new names the object", "[objects]") {
    auto script = harness(R"(
        function main() {
            temp.widget = new Widget(MyWidget) {
                text = tag;
            };

            return temp.widget.text;
        }
    )");

    script.env.register_type(
        prototype_builder<widget>("Widget")
            .constructor([](const values &args) {
                auto made = make_shared<widget>();
                made->text = args.empty() ? string{} : to_string(args[0]);

                return made;
            })
            .property(
                "tag",
                [](const widget *self) { return self->text; })
            .property(
                "text",
                [](const widget *self) { return self->text; },
                [](widget *self, const string &value) { self->text = value; })
            .build());

    REQUIRE(script.text("main") == "MyWidget");
}

TEST_CASE("a resolvable name in new is still passed by value", "[objects]") {
    auto script = harness(R"(
        function main() {
            temp.chosen = "FromVariable";
            temp.widget = new Widget(temp.chosen);

            return temp.widget.text;
        }
    )");

    script.env.register_type(
        prototype_builder<widget>("Widget")
            .constructor([](const values &args) {
                auto made = make_shared<widget>();
                made->text = args.empty() ? string{} : to_string(args[0]);

                return made;
            })
            .property(
                "text",
                [](const widget *self) { return self->text; },
                [](widget *self, const string &value) { self->text = value; })
            .build());

    REQUIRE(script.text("main") == "FromVariable");
}

TEST_CASE("a new body may contain statements", "[objects]") {
    auto script = harness(R"(
        function main() {
            temp.widget = new Widget() {
                width = 0;

                for (temp.i = 0; temp.i < 4; temp.i++) {
                    width = width + 2;
                }

                if (width > 4) {
                    height = 3;
                } else {
                    height = 1;
                }
            };

            return temp.widget.area();
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 24.0);
}

TEST_CASE("a new body can build children in a loop", "[objects]") {
    auto script = harness(R"(
        function main() {
            temp.total = 0;

            temp.parent = new Widget() {
                width = 1;
                height = 1;

                for (temp.i = 0; temp.i < 3; temp.i++) {
                    temp.child = new Widget() {
                        width = temp.i;
                        height = 2;
                    };

                    temp.total = temp.total + temp.child.area();
                }
            };

            return temp.total + temp.parent.area();
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 7.0);
}

TEST_CASE("a break inside a new body does not escape it", "[objects]") {
    auto script = harness(R"(
        function main() {
            temp.count = 0;

            for (temp.i = 0; temp.i < 3; temp.i++) {
                temp.widget = new Widget() {
                    width = 1;
                    height = 1;

                    for (temp.j = 0; temp.j < 5; temp.j++) {
                        break;
                    }
                };

                temp.count = temp.count + 1;
            }

            return temp.count;
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 3.0);
}

TEST_CASE("a named object resolves as a global afterwards", "[objects]") {
    auto script = harness(R"(
        function build() {
            new Widget("StatusPanel") {
                width = 5;
                height = 4;
            };
        }

        function main() {
            build();

            StatusPanel.width = 10;

            return StatusPanel.area();
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 40.0);
}

TEST_CASE("a named object is addressable in any case", "[objects]") {
    auto script = harness(R"(
        function main() {
            new Widget("StatusPanel") {
                width = 3;
                height = 3;
            };

            return statuspanel.area();
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 9.0);
}

TEST_CASE("script locals win over an object of the same name", "[objects]") {
    auto script = harness(R"(
        function main() {
            new Widget("marker") {
                width = 2;
                height = 2;
            };

            marker = 7;

            return marker;
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 7.0);
}

TEST_CASE("clearing the object registry drops the names", "[objects]") {
    auto script = harness(R"(
        function build() {
            new Widget("Panel") {
                width = 2;
                height = 2;
            };
        }

        function area() {
            return Panel.area();
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(to_number(*script.call("build")) == 0.0);
    REQUIRE(script.number("area") == 4.0);

    script.env.clear_objects();

    REQUIRE(script.number("area") == 0.0);
}

TEST_CASE("a bare name in with addresses the object's property", "[objects]") {
    auto script = harness(R"(
        function main() {
            temp.widget = new Widget();

            with (temp.widget) {
                width = 6;
                height = width + 1;
            }

            return temp.widget.area();
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.number("main") == 42.0);
}

TEST_CASE("with on a non-object skips the block", "[objects]") {
    auto script = harness(R"(
        function main() {
            this.reached = 0;

            with (temp.missing) {
                this.reached = 1;
            }

            return this.reached;
        }
    )");

    REQUIRE(script.number("main") == 0.0);
}

TEST_CASE("public functions are callable, private ones are not", "[objects]") {
    auto script = harness(R"(
        public function open() {
            return 1;
        }

        function hidden() {
            return 2;
        }
    )");

    REQUIRE(script.script->is_public("open"));
    REQUIRE_FALSE(script.script->is_public("hidden"));

    REQUIRE(to_number(*script.script->call_public("open", script.self)) == 1.0);

    const auto refused = script.script->call_public("hidden", script.self);

    REQUIRE_FALSE(refused);
    REQUIRE(refused.error().message.contains("not a public function"));
}

TEST_CASE("TStaticVar is a bag of variables", "[objects]") {
    auto script = harness(R"(
        function fields() {
            temp.bag = new TStaticVar();
            temp.bag.name = "carol";

            return temp.bag.name;
        }

        function loads() {
            temp.bag = new TStaticVar();
            temp.bag.loadvarsfromarray("level=5,class=Novice");

            return temp.bag.class @ "/" @ temp.bag.level;
        }
    )");

    REQUIRE(script.text("fields") == "carol");
    REQUIRE(script.text("loads") == "Novice/5");
}

TEST_CASE("tearing down something that was never built is not an error", "[objects]") {
    auto script = harness(R"(
        function teardown() {
            temp.missing.destroy();
            temp.missing.hide();

            return "survived";
        }

        function stillReports() {
            temp.missing.notafunction();

            return "unreachable";
        }
    )");

    REQUIRE(script.text("teardown") == "survived");
    REQUIRE_FALSE(script.call("stillReports"));
}

TEST_CASE("an initializer body still sees the loop that builds the object", "[objects]") {
    auto script = harness(R"(
        function build() {
            this.labels = {"first", "second", "third"};
            temp.seen = NULL;

            for (i = 0; i < this.labels.size(); i++) {
                temp.made = new Widget() {
                    text = thiso.labels[i];
                    width = i;
                };

                temp.seen.add(temp.made.text @ ":" @ temp.made.width);
            }

            return temp.seen;
        }
    )");

    script.env.register_type(widget_prototype());

    REQUIRE(script.text("build") == "first:0,second:1,third:2");
}
