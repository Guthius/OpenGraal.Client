add_library(graal-net
        acceptor.cpp
        acceptor.hpp
        connection.cpp
        connection.hpp
        message.cpp
        message.hpp
)

target_include_directories(graal-net PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(graal-net PUBLIC asio)
