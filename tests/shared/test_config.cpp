#include <catch2/catch_test_macros.hpp>

#include <shared/config.hpp>

using namespace og::shared;

TEST_CASE("folder patterns match within one segment", "[config]") {
    REQUIRE(match_folder_pattern("*.nw", "town.nw"));
    REQUIRE(match_folder_pattern("*.nw", "TOWN.NW"));
    REQUIRE(match_folder_pattern("gicon_*.png", "gicon_knights.png"));
    REQUIRE(match_folder_pattern("t?wn.nw", "town.nw"));

    REQUIRE_FALSE(match_folder_pattern("*.nw", "town.png"));
    REQUIRE_FALSE(match_folder_pattern("*.nw", "insides/town.nw"));
    REQUIRE_FALSE(match_folder_pattern("t?wn.nw", "t/wn.nw"));
}

TEST_CASE("folder patterns with directories match them literally", "[config]") {
    REQUIRE(match_folder_pattern("insides/*.nw", "insides/town.nw"));
    REQUIRE(match_folder_pattern("images/tiles/*.png", "images/tiles/pics1.png"));

    REQUIRE_FALSE(match_folder_pattern("insides/*.nw", "town.nw"));
    REQUIRE_FALSE(match_folder_pattern("insides/*.nw", "insides/deep/town.nw"));
    REQUIRE_FALSE(match_folder_pattern("images/*.png", "images/tiles/pics1.png"));
}

TEST_CASE("empty and degenerate patterns behave", "[config]") {
    REQUIRE(match_folder_pattern("*", "anything.nw"));
    REQUIRE(match_folder_pattern("**", "anything.nw"));
    REQUIRE(match_folder_pattern("town.nw", "town.nw"));

    REQUIRE_FALSE(match_folder_pattern("", "town.nw"));
    REQUIRE_FALSE(match_folder_pattern("*", "insides/town.nw"));
    REQUIRE_FALSE(match_folder_pattern("town.nw", "town.nwx"));
}
