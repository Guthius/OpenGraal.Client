include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.0
)

FetchContent_MakeAvailable(Catch2)

set(OPENGRAAL_TEST_WORLD_PATH "${CMAKE_SOURCE_DIR}/../Thamhic_Release" CACHE PATH
        "Path to a real Graal gameworld the tests read as test data")

add_executable(opengraal-tests
        gs2/script_harness.hpp
        gs2/test_case_insensitivity.cpp
        gs2/test_eval_assign_expr.cpp
        gs2/test_eval_call_expr.cpp
        gs2/test_eval_literals.cpp
        gs2/test_join.cpp
        gs2/test_objects.cpp
        gs2/test_operators.cpp
        gs2/test_scoping.cpp
        gs2/test_sample_scripts.cpp
        gs2/test_stdlib.cpp
        gs2/utils.hpp
        server/test_account.cpp
        server/test_scripts.cpp
        server/test_world_features.cpp
        server/test_session.cpp
        shared/test_config.cpp
        shared/test_sample_world.cpp
        shared/test_gani.cpp
        shared/test_gmap.cpp
        shared/test_level.cpp
        shared/test_level_writer.cpp
        shared/test_text.cpp
        shared/test_tile_defs.cpp
        shared/test_token.cpp
)

target_compile_definitions(opengraal-tests PRIVATE
        OPENGRAAL_TEST_WORLD_PATH="${OPENGRAAL_TEST_WORLD_PATH}"
        OPENGRAAL_ASSET_PATH="${CMAKE_SOURCE_DIR}/bin"
)

target_link_libraries(opengraal-tests PRIVATE Catch2::Catch2WithMain graal-gs2 graal-net graal-server graal-shared)

include(CTest)
include(Catch)

catch_discover_tests(opengraal-tests)
