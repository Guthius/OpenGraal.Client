#include <gs2/environment.hpp>
#include <gs2/script.hpp>
#include <gs2/value.hpp>

#include <filesystem>
#include <iostream>
#include <print>

using namespace std;

auto main(int argc, char *argv[]) -> int {
    if (argc != 2) {
        println(cerr, "No input file specified");
        return 1;
    }

    auto path = filesystem::path(argv[1]);
    if (!filesystem::exists(path)) {
        println(cerr, "Input file not found: {}", path.string());
        return 1;
    }

    auto script_scope = make_shared<og::gs2::basic_dictionary>();
    auto script_env = og::gs2::environment();
    auto script = og::gs2::compile(script_env, path);

    if (!script) {
        println(cerr, "Error: {} on line {}, column {}",
            script.error().message,
            script.error().position.line,
            script.error().position.column);
        return 1;
    }

    script_env.register_function("print", [](auto &env, const auto &args) -> og::gs2::expected_value {
        for (const auto &arg : args) {
            cout << og::gs2::to_string(arg);
        }

        return {};
    });

    auto result = (*script)->call("onCreated", script_scope);
    if (!result) {
        println(cerr, "Error: {} on line {}, column {}",
            result.error().message,
            result.error().position.line,
            result.error().position.column);
        return 1;
    }

    return 0;
}
