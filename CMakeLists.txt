cmake_minimum_required(VERSION 3.26)
project(OpenGraal)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib CONFIG REQUIRED)

if (MINGW)
    add_link_options(-mwindows -static-libstdc++ -static-libgcc)
endif()

add_executable(OpenGraal WIN32
        src/actor.cpp
        src/actor.hpp
        src/animation.cpp
        src/animation.hpp
        src/animation_manager.cpp
        src/animation_manager.hpp
        src/constants.hpp
        src/file_manager.cpp
        src/file_manager.hpp
        src/game.cpp
        src/game.hpp
        src/leaps.hpp
        src/leaps.cpp
        src/level.cpp
        src/level.hpp
        src/level_manager.cpp
        src/level_manager.hpp
        src/level_sign.cpp
        src/level_sign.hpp
        src/main.cpp
        src/player.cpp
        src/player.hpp
        src/sign.cpp
        src/sign.hpp
        src/sound_manager.cpp
        src/sound_manager.hpp
        src/texture_manager.cpp
        src/texture_manager.hpp
        src/thrown_item.hpp
        src/thrown_item.cpp
        src/tileset.cpp
        src/tileset.hpp
        src/tileset_manager.cpp
        src/tileset_manager.hpp
        src/utils.cpp
        src/utils.hpp
)

target_sources(OpenGraal PRIVATE game.rc)
target_link_libraries(OpenGraal PRIVATE raylib)
