#include <server/server.hpp>
#include <shared/text.hpp>

#include <cstdio>
#include <iostream>
#include <print>

using namespace std;
using namespace og;

namespace {
    auto parse_options(const int argc, char *argv[]) -> server::server_options {
        server::server_options options;

        for (int i = 1; i < argc; ++i) {
            const auto argument = string_view(argv[i]);

            if (argument.starts_with("--port=")) {
                options.port = static_cast<uint16_t>(shared::to_int(argument.substr(7), options.port));
            } else if (argument == "--no-registration") {
                options.allow_registration = false;
            } else if (!argument.starts_with("--")) {
                options.world_path = argument;
            }
        }

        return options;
    }
}

auto main(const int argc, char *argv[]) -> int {
    setvbuf(stdout, nullptr, _IOLBF, 0);

    auto options = parse_options(argc, argv);
    auto instance = server::server(options);

    if (const auto started = instance.start(); !started) {
        println(cerr, "{}", started.error());
        return 1;
    }

    println("OpenGraal server listening on port {}", instance.port());
    println("World: {}", options.world_path.string());
    println("Start level: {} ({}, {})",
        instance.get_world().start().level,
        instance.get_world().start().x,
        instance.get_world().start().y);

    instance.run();

    return 0;
}
