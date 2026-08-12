#include <catch2/catch_test_macros.hpp>

#include <shared/level.hpp>
#include <shared/sign_text.hpp>

#include <filesystem>
#include <format>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace og::shared;

namespace {
    const auto test_world = filesystem::path(OPENGRAAL_TEST_WORLD_PATH);

    auto parse(const string &text, const string &name = "test.nw") -> level_data {
        istringstream is(text);
        auto level = load_level(name, is);

        REQUIRE(level);

        return *level;
    }

    // A 12-bit little-endian code stream, the GR-V1.00 board encoding.
    auto pack_codes(const vector<int> &codes) -> string {
        string bytes;
        unsigned int buffer = 0;
        int bits = 0;

        for (const auto code : codes) {
            buffer |= static_cast<unsigned int>(code) << bits;
            bits += 12;

            while (bits >= 8) {
                bytes.push_back(static_cast<char>(buffer & 0xFF));
                buffer >>= 8;
                bits -= 8;
            }
        }

        if (bits > 0) {
            bytes.push_back(static_cast<char>(buffer & 0xFF));
        }

        return bytes;
    }

    auto graal_fixture() -> string {
        vector<int> codes;
        for (int i = 0; i < 16; ++i) {
            codes.push_back(0x800 | 255);
            codes.push_back(77);
        }
        codes.push_back(0x800 | 16);
        codes.push_back(77);

        auto data = string("GR-V1.00") + pack_codes(codes);
        data += "somewhere.nw 10 11 2 2 playerx playery\n#\n";
        data += "\x07\x08\x03";
        data += "verse one\\verse two\n";
        data += "\xff\xff\xff\n";
        data += string() + static_cast<char>(40 + 32) + static_cast<char>(26 + 32) + "npc1.png#say hello\n\n";
        data += string() + static_cast<char>(12 + 32) + static_cast<char>(13 + 32) + encode_sign_text("hello") + "\n";

        return data;
    }
}

TEST_CASE("a saved level parses back equal and re-saves identically", "[level-writer]") {
    const auto original = parse(
        "GLEVNW01\n"
        "BOARD 0 0 64 0 " +
        string(128, 'B') +
        "\n"
        "LINK my level.nw 30 31 2 3 playerx playery\n"
        "CHEST 5 6 greenrupee 2\n"
        "SIGN 10 11\n"
        "line one\n"
        "line two\n"
        "SIGNEND\n"
        "NPC door open.png 41 19.5\n"
        "join nopking;\n"
        "NPCEND\n"
        "BADDY 7 8 3\n"
        "verse one\n"
        "BADDYEND\n");

    const auto saved = save_level_text(original);
    const auto reparsed = parse(saved);

    REQUIRE(reparsed.layers.size() == 1);
    REQUIRE(*reparsed.tiles() == *original.tiles());
    REQUIRE(reparsed.links.size() == 1);
    REQUIRE(reparsed.links[0].destination == "my level.nw");
    REQUIRE(reparsed.links[0].destination_x == "playerx");
    REQUIRE(reparsed.links[0].destination_y == "playery");
    REQUIRE(reparsed.chests.size() == 1);
    REQUIRE(reparsed.chests[0].item == "greenrupee");
    REQUIRE(reparsed.signs.size() == 1);
    REQUIRE(reparsed.signs[0].text == "line one\nline two\n");
    REQUIRE(reparsed.npcs.size() == 1);
    REQUIRE(reparsed.npcs[0].image == "door open.png");
    REQUIRE(reparsed.npcs[0].x == 41.0f);
    REQUIRE(reparsed.npcs[0].y == 19.5f);
    REQUIRE(reparsed.npcs[0].script == "join nopking;\n");
    REQUIRE(reparsed.baddies.size() == 1);
    REQUIRE(reparsed.baddies[0].verses == vector<string>{"verse one"});

    REQUIRE(save_level_text(reparsed) == saved);
}

TEST_CASE("an imageless npc round-trips through the '-' placeholder", "[level-writer]") {
    const auto original = parse(
        "GLEVNW01\n"
        "NPC - 4 5\n"
        "NPCEND\n");

    const auto reparsed = parse(save_level_text(original));

    REQUIRE(reparsed.npcs.size() == 1);
    REQUIRE(reparsed.npcs[0].image.empty());
    REQUIRE(reparsed.npcs[0].script.empty());
}

TEST_CASE("heights blocks and unknown lines survive a save", "[level-writer]") {
    const auto original = parse(
        "GLEVNW01\n"
        "HEIGHTS\n"
        "1 2 3 4\n"
        "HEIGHTSEND\n"
        "FUTUREBLOCK something we do not understand\n");

    REQUIRE(original.raw_lines ==
            vector<string>{"HEIGHTS", "1 2 3 4", "HEIGHTSEND", "FUTUREBLOCK something we do not understand"});

    const auto saved = save_level_text(original);

    REQUIRE(saved.contains("HEIGHTS\n1 2 3 4\nHEIGHTSEND\n"));
    REQUIRE(saved.contains("FUTUREBLOCK something we do not understand\n"));
    REQUIRE(parse(saved).raw_lines == original.raw_lines);
}

TEST_CASE("overlay layers write ascending, content rows only", "[level-writer]") {
    const auto original = parse(
        "GLEVNW01\n"
        "BOARD 0 9 64 5 " +
        string(128, 'C') +
        "\n"
        "BOARD 0 0 64 1 " +
        string(128, 'B') + "\n");

    const auto saved = save_level_text(original);
    const auto layer1 = saved.find(" 64 1 ");
    const auto layer5 = saved.find(" 64 5 ");

    REQUIRE(layer1 != string::npos);
    REQUIRE(layer5 != string::npos);
    REQUIRE(layer1 < layer5);

    const auto reparsed = parse(saved);

    REQUIRE(reparsed.layers.size() == 2);
    REQUIRE(reparsed.find_layer(1)->tiles == original.find_layer(1)->tiles);
    REQUIRE(reparsed.find_layer(5)->tiles == original.find_layer(5)->tiles);

    auto rows = 0;
    for (auto at = saved.find(" 64 5 "); at != string::npos; at = saved.find(" 64 5 ", at + 1)) {
        ++rows;
    }

    REQUIRE(rows == 1);
}

TEST_CASE("a binary graal level saves as GLEVNW01", "[level-writer]") {
    const auto original = parse(graal_fixture(), "old.graal");

    REQUIRE(original.version == "GR-V1.00");

    const auto saved = save_level_text(original);

    REQUIRE(saved.starts_with("GLEVNW01\n"));

    const auto reparsed = parse(saved);

    REQUIRE(*reparsed.tiles() == *original.tiles());
    REQUIRE((*reparsed.tiles())[0] == 77);
    REQUIRE((*reparsed.tiles())[4095] == 77);
    REQUIRE(reparsed.links.size() == 1);
    REQUIRE(reparsed.links[0].destination == "somewhere.nw");
    REQUIRE(reparsed.baddies.size() == 1);
    REQUIRE(reparsed.baddies[0].verses == vector<string>{"verse one", "verse two"});
    REQUIRE(reparsed.npcs.size() == 1);
    REQUIRE(reparsed.npcs[0].image == "npc1.png");
    REQUIRE(reparsed.npcs[0].x == 40.0f);
    REQUIRE(reparsed.npcs[0].script == "say hello\n");
    REQUIRE(reparsed.signs.size() == 1);
    REQUIRE(reparsed.signs[0].x == 12);
    REQUIRE(reparsed.signs[0].text == "hello\n");
}

TEST_CASE("every level in the test world saves stably", "[level-writer]") {
    if (!is_directory(test_world / "levels")) {
        SKIP(format("no levels under {}", (test_world / "levels").string()));
    }

    vector<string> failures;
    auto count = 0;

    for (const auto &entry : filesystem::recursive_directory_iterator(test_world / "levels")) {
        if (!entry.is_regular_file() || entry.path().extension() != ".nw") {
            continue;
        }

        ++count;

        const auto first = load_level(entry.path());
        if (!first) {
            failures.push_back(format("{}: {}", entry.path().filename().string(), first.error()));
            continue;
        }

        const auto once = save_level_text(*first);

        istringstream is(once);
        const auto second = load_level(first->name, is);
        if (!second) {
            failures.push_back(format("{}: reparse failed: {}", first->name, second.error()));
            continue;
        }

        if (save_level_text(*second) != once) {
            failures.push_back(format("{}: save is not stable", first->name));
        }
    }

    if (count == 0) {
        SKIP("no .nw levels in the test world");
    }

    INFO(format("{} of {} failed:\n{}", failures.size(), count, [&failures] {
        string joined;
        for (const auto &failure : failures) {
            joined += "  " + failure + "\n";
        }
        return joined;
    }()));

    REQUIRE(failures.empty());
}
