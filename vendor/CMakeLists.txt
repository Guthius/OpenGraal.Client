include(FetchContent)

# Raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5   # use the version you want
)

FetchContent_MakeAvailable(raylib)

# Asio (standalone, header only)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(asio)

add_library(asio INTERFACE)

target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)

target_compile_definitions(asio INTERFACE
        ASIO_STANDALONE
        ASIO_NO_DEPRECATED
)

find_package(Threads REQUIRED)

target_link_libraries(asio INTERFACE Threads::Threads)

if (WIN32)
    target_link_libraries(asio INTERFACE ws2_32 wsock32)
endif ()

# Argon2 (password hashing, and the keyed BLAKE2b the session token is signed with)
FetchContent_Declare(
    argon2
    GIT_REPOSITORY https://github.com/P-H-C/phc-winner-argon2.git
    GIT_TAG 20190702
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(argon2)

add_library(argon2 STATIC
        ${argon2_SOURCE_DIR}/src/argon2.c
        ${argon2_SOURCE_DIR}/src/core.c
        ${argon2_SOURCE_DIR}/src/encoding.c
        ${argon2_SOURCE_DIR}/src/ref.c
        ${argon2_SOURCE_DIR}/src/thread.c
        ${argon2_SOURCE_DIR}/src/blake2/blake2b.c
)

target_include_directories(argon2 PUBLIC
        ${argon2_SOURCE_DIR}/include
        ${argon2_SOURCE_DIR}/src
)

target_compile_options(argon2 PRIVATE -w)

target_link_libraries(argon2 PRIVATE Threads::Threads)
