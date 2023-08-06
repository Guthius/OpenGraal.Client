cmake_minimum_required(VERSION 3.26)
project(Terestria)

find_package(raylib CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)

add_executable(Terestria src/main.cpp
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
        src/Actors/Actor.h
        src/Utils.cpp
        src/Utils.h
        src/Actors/Actor.cpp
        src/Constants.h
        src/Resources/SoundManager.cpp
        src/Resources/SoundManager.h
        src/AnimationManager.cpp
        src/AnimationManager.h
        src/Actors/Player.cpp
        src/Actors/Player.h
        src/LevelManager.cpp
        src/LevelManager.h
        src/FileManager.cpp
        src/FileManager.h)

target_link_libraries(Terestria PRIVATE raylib)
