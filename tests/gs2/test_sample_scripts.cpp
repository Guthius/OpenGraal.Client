#include <catch2/catch_test_macros.hpp>

#include <gs2/environment.hpp>
#include <gs2/script.hpp>
#include <shared/container.hpp>
#include <shared/level.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace og::gs2;

namespace {
    constexpr size_t max_gs2_failures = 0;
    constexpr size_t max_client_failures = 0;
    constexpr size_t max_level_npc_failures = 0;

    const auto test_world = filesystem::path(OPENGRAAL_TEST_WORLD_PATH);

    struct source_file {
        string name;
        string source;
    };

    struct parse_report {
        size_t total = 0;
        vector<string> failures;
    };

    auto files_in(const filesystem::path &directory, const string_view extension) -> vector<filesystem::path> {
        vector<filesystem::path> paths;

        if (!is_directory(directory)) {
            return paths;
        }

        for (const auto &entry : filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == extension) {
                paths.push_back(entry.path());
            }
        }

        return paths;
    }

    auto parse_all(const vector<source_file> &sources) -> parse_report {
        parse_report report;
        environment env;

        for (const auto &[name, source] : sources) {
            if (source.find_first_not_of(" \t\r\n") == string::npos) {
                continue;
            }

            ++report.total;

            istringstream is(source);
            if (auto script = load_script(env, is); !script) {
                report.failures.push_back(format("{}: {} (line {})", name, script.error().message, script.error().position.line));
            }
        }

        return report;
    }

    void report_rate(const string_view what, const parse_report &report, const size_t allowed) {
        const auto rate = report.total == 0
                              ? 0.0
                              : 100.0 * static_cast<double>(report.failures.size()) / static_cast<double>(report.total);

        string detail;
        for (size_t i = 0; i < report.failures.size() && i < 15; ++i) {
            detail += "  " + report.failures[i] + "\n";
        }

        INFO(format("{}: {} of {} failed ({:.1f}%), allowed {}\n{}",
            what,
            report.failures.size(),
            report.total,
            rate,
            allowed,
            detail));

        REQUIRE(report.failures.size() <= allowed);
    }

    auto half_of(const string &source, const bool client) -> string {
        auto halves = og::gs2::split_script(source);

        return client ? std::move(halves.client) : std::move(halves.server);
    }

    auto gs2_sources(const bool client) -> vector<source_file> {
        vector<source_file> sources;

        for (const auto &path : files_in(test_world / "weapons", ".txt")) {
            if (const auto weapon = og::shared::load_weapon(path)) {
                sources.push_back({path.filename().string(), half_of(weapon->script, client)});
            }
        }

        for (const auto &path : files_in(test_world / "npcs", ".txt")) {
            if (const auto npc = og::shared::load_npc(path)) {
                sources.push_back({path.filename().string(), half_of(npc->script, client)});
            }
        }

        for (const auto &path : files_in(test_world / "scripts", ".txt")) {
            ifstream ifs(path);
            ostringstream buffer;
            buffer << ifs.rdbuf();

            sources.push_back({path.filename().string(), half_of(buffer.str(), client)});
        }

        return sources;
    }

    // GraalScript v1 is out of scope (PLAN.md). Level NPCs written for it use `if (created)`
    // blocks and bare commands instead of function declarations; skip them by that signature.
    auto is_legacy_script(const string_view source) -> bool {
        return !source.contains("function ");
    }

    auto level_npc_sources() -> vector<source_file> {
        vector<source_file> sources;

        for (const auto &path : files_in(test_world / "levels", ".nw")) {
            const auto level = og::shared::load_level(path);
            if (!level) {
                continue;
            }

            for (size_t i = 0; i < level->npcs.size(); ++i) {
                auto source = og::gs2::split_script(level->npcs[i].script).server;
                if (is_legacy_script(source)) {
                    continue;
                }

                sources.push_back({
                    .name = format("{}#{}", path.filename().string(), i),
                    .source = std::move(source),
                });
            }
        }

        return sources;
    }
}

TEST_CASE("every weapon, npc and class script parses", "[world][gs2]") {
    const auto sources = gs2_sources(false);
    if (sources.empty()) {
        SKIP(format("no scripts under {}", test_world.string()));
    }

    report_rate("gs2 scripts", parse_all(sources), max_gs2_failures);
}

TEST_CASE("the client half of every script parses", "[world][gs2]") {
    const auto sources = gs2_sources(true);
    if (sources.empty()) {
        SKIP(format("no scripts under {}", test_world.string()));
    }

    report_rate("gs2 client scripts", parse_all(sources), max_client_failures);
}

TEST_CASE("gs2 level npc scripts parse", "[world][gs2]") {
    const auto sources = level_npc_sources();
    if (sources.empty()) {
        SKIP("no gs2 level npcs in the test world");
    }

    report_rate("gs2 level npcs", parse_all(sources), max_level_npc_failures);
}

TEST_CASE("the shared class scripts load and run pure functions", "[world][gs2]") {
    const auto path = test_world / "scripts" / "mudfunctions.txt";
    if (!exists(path)) {
        SKIP("mudfunctions.txt not present");
    }

    environment env;
    ifstream ifs(path);
    ostringstream buffer;
    buffer << ifs.rdbuf();

    istringstream is(og::gs2::split_script(buffer.str()).server);

    const auto script = load_script(env, is);

    REQUIRE(script);
    REQUIRE((*script)->has_function("Mud_FindArch"));
    REQUIRE((*script)->is_public("Mud_SaveAccount"));
    REQUIRE_FALSE((*script)->is_public("Mud_FindArch"));
}
