add_library(graal-shared
        config.cpp
        config.hpp
        container.cpp
        container.hpp
        gani.cpp
        gani.hpp
        gmap.cpp
        gmap.hpp
        level.cpp
        level.hpp
        result.cpp
        result.hpp
        random.cpp
        random.hpp
        sign_text.cpp
        sign_text.hpp
        text.cpp
        text.hpp
        terrain.cpp
        tile_defs.cpp
        tile_defs.hpp
        token.cpp
        token.hpp
)

target_include_directories(graal-shared PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(graal-shared PRIVATE argon2)
