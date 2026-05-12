cmake_minimum_required(VERSION 4.1)
project(OpenGraal)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor)

add_executable(OpenGraal WIN32
        src/gui/gui_button_base_ctrl.cpp
        src/gui/gui_button_base_ctrl.hpp
        src/gui/gui_button_ctrl.cpp
        src/gui/gui_button_ctrl.hpp
        src/gui/gui_check_box_ctrl.cpp
        src/gui/gui_check_box_ctrl.hpp
        src/gui/gui_control.cpp
        src/gui/gui_control.hpp
        src/gui/gui_control_profile.cpp
        src/gui/gui_control_profile.hpp
        src/gui/gui_radio_ctrl.cpp
        src/gui/gui_radio_ctrl.hpp
        src/gui/gui_text_ctrl.cpp
        src/gui/gui_text_ctrl.hpp
        src/gui/gui_text_edit_ctrl.cpp
        src/gui/gui_text_edit_ctrl.hpp
        src/gui/gui_utils.hpp
        src/gui/gui_window_ctrl.cpp
        src/gui/gui_window_ctrl.hpp

        src/actor.cpp
        src/actor.hpp
        src/actor_move.cpp
        src/animation.cpp
        src/animation.hpp
        src/animation_manager.cpp
        src/animation_manager.hpp
        src/carry_object.cpp
        src/carry_object.hpp
        src/constants.hpp
        src/file_manager.cpp
        src/file_manager.hpp
        src/font_manager.cpp
        src/font_manager.hpp
        src/game.cpp
        src/game.hpp
        src/input.cpp
        src/input.hpp
        src/leap_effect.hpp
        src/leap_effect.cpp
        src/level.cpp
        src/level.hpp
        src/level_link.cpp
        src/level_link.hpp
        src/level_manager.cpp
        src/level_manager.hpp
        src/level_sign.cpp
        src/level_sign.hpp
        src/main.cpp
        src/player.cpp
        src/player.hpp
        src/player_state.cpp
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

target_link_libraries(OpenGraal PRIVATE raylib glfw)

target_compile_options(OpenGraal PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-Wno-changes-meaning>
)
