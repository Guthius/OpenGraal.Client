add_library(graal-server
        account.cpp
        account.hpp
        script_host.cpp
        script_host.hpp
        server.cpp
        server.hpp
        world.cpp
        world.hpp
)

target_link_libraries(graal-server PUBLIC graal-shared graal-gs2 graal-net PRIVATE argon2)

add_executable(OpenGraalServer
        main.cpp
)

target_link_libraries(OpenGraalServer PRIVATE graal-server)
