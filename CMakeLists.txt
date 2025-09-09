cmake_minimum_required(VERSION 3.26)
project(OpenGraal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib CONFIG REQUIRED)

if (MINGW)
    add_link_options(-static-libstdc++ -static-libgcc)
endif()

add_executable(OpenGraal
        src/main.cpp
        src/Level.cpp
        src/Level.h
        src/Tileset.cpp
        src/Tileset.h
        src/Game.cpp
        src/Game.h
        src/Animation.cpp
        src/Animation.h
        src/TextureManager.cpp
        src/TextureManager.h
        src/Actor.h
        src/Utils.cpp
        src/Utils.h
        src/Actor.cpp
        src/Constants.h
        src/SoundManager.cpp
        src/SoundManager.h
        src/AnimationManager.cpp
        src/AnimationManager.h
        src/Player.cpp
        src/Player.h
        src/LevelManager.cpp
        src/LevelManager.h
        src/FileManager.cpp
        src/FileManager.h
        src/LevelSign.cpp
        src/LevelSign.h
        src/TilesetManager.cpp
        src/TilesetManager.h
        src/Sign.cpp
        src/Sign.h)

target_sources(OpenGraal PRIVATE game.rc)
target_link_libraries(OpenGraal PRIVATE raylib)
