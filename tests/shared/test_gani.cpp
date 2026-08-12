#include <catch2/catch_test_macros.hpp>

#include <shared/gani.hpp>

#include <filesystem>
#include <sstream>

using namespace std;
using namespace og::shared;

TEST_CASE("the GANI0001 header is optional", "[gani]") {
    istringstream is(
        "SPRITE 0 SPRITES 0 0 24 12 shadow\r\n"
        "SINGLEDIRECTION\r\n");

    const auto gani = load_gani("headerless.gani", is);

    REQUIRE(gani);
    REQUIRE(gani->single_direction);
    REQUIRE(gani->find_sprite(0) != nullptr);
}

TEST_CASE("sprite sources are classified", "[gani]") {
    istringstream is(
        "GANI0001\n"
        "SPRITE 0 SPRITES 0 0 24 12 shadow\n"
        "SPRITE 1 ATTR7 1 2 3 4 hat\n"
        "SPRITE 2 tree.png 5 6 7 8 a tree\n");

    const auto gani = load_gani("sprites.gani", is);

    REQUIRE(gani);
    REQUIRE(gani->sprites.size() == 3);

    REQUIRE(gani->find_sprite(0)->source == gani_sprite_source::sprites);
    REQUIRE(gani->find_sprite(0)->width == 24);
    REQUIRE(gani->find_sprite(0)->description == "shadow");

    REQUIRE(gani->find_sprite(1)->source == gani_sprite_source::attribute);
    REQUIRE(gani->find_sprite(1)->source_index == 7);

    REQUIRE(gani->find_sprite(2)->source == gani_sprite_source::file);
    REQUIRE(gani->find_sprite(2)->image == "tree.png");
    REQUIRE(gani->find_sprite(2)->description == "a tree");
}

TEST_CASE("four direction frames are read per animation step", "[gani]") {
    istringstream is(
        "GANI0001\n"
        "SPRITE 0 SPRITES 0 0 16 16 a\n"
        "ANI\n"
        "0 1 2\n"
        "0 3 4\n"
        "0 5 6\n"
        "0 7 8\n"
        "WAIT 3\n"
        "PLAYSOUND step.wav 1 2\n"
        "\n"
        "ANIEND\n");

    const auto gani = load_gani("walk.gani", is);

    REQUIRE(gani);
    REQUIRE(gani->frames.size() == 1);

    const auto &frame = gani->frames.front();

    REQUIRE(frame.directions[0].size() == 1);
    REQUIRE(frame.directions[0][0].x == 1.0f);
    REQUIRE(frame.directions[3][0].y == 8.0f);
    REQUIRE(frame.duration == 0.3f);
    REQUIRE(frame.sound == "step.wav");
}

TEST_CASE("single direction animations read one row per frame", "[gani]") {
    istringstream is(
        "GANI0001\n"
        "SPRITE 0 shadow.png 0 0 8 8 s\n"
        "SPRITE 1 tree.png 0 0 8 8 t\n"
        "SINGLEDIRECTION\n"
        "DEFAULTHEAD head19.png\n"
        "ANI\n"
        "0 -111 -25, 1 -180 -298\n"
        "ANIEND\n");

    const auto gani = load_gani("tree.gani", is);

    REQUIRE(gani);
    REQUIRE(gani->single_direction);
    REQUIRE(gani->default_head == "head19.png");
    REQUIRE(gani->frames.size() == 1);
    REQUIRE(gani->frames[0].directions[0].size() == 2);
    REQUIRE(gani->frames[0].directions[1].empty());
}

TEST_CASE("attachments, loops and default attributes are read", "[gani]") {
    istringstream is(
        "GANI0001\n"
        "SPRITE 0 SPRITES 0 0 8 8 a\n"
        "ATTACHSPRITE 1 400 -4 -8\n"
        "LOOP\n"
        "CONTINUOUS\n"
        "SETBACKTO idle\n"
        "DEFAULTATTR1 hat0.png\n"
        "DEFAULTATTR12 cape.png\n");

    const auto gani = load_gani("attach.gani", is);

    REQUIRE(gani);
    REQUIRE(gani->loop);
    REQUIRE(gani->continuous);
    REQUIRE(gani->set_back_to == "idle");
    REQUIRE(gani->attachments.size() == 1);
    REQUIRE(gani->attachments[0].attached_sprite_id == 400);
    REQUIRE(gani->attachments[0].y == -8.0f);
    REQUIRE(gani->default_attributes.at(1) == "hat0.png");
    REQUIRE(gani->default_attributes.at(12) == "cape.png");
}

TEST_CASE("the bundled ganis load", "[gani]") {
    const auto directory = filesystem::path(OPENGRAAL_ASSET_PATH) / "levels" / "ganis";
    if (!is_directory(directory)) {
        SKIP("bundled ganis not present");
    }

    for (const auto &entry : filesystem::directory_iterator(directory)) {
        if (entry.path().extension() != ".gani") {
            continue;
        }

        INFO(entry.path().string());
        REQUIRE(load_gani(entry.path()));
    }
}
