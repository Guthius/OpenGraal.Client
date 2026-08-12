add_library(graal-gs2
        ast.cpp
        ast.hpp
        basic_dictionary.hpp
        builtins.cpp
        builtins.hpp
        context.cpp
        context.hpp
        environment.cpp
        environment.hpp
        error.hpp
        eval.cpp
        eval.hpp
        lexer.cpp
        lexer.hpp
        parser.cpp
        parser.hpp
        prototype.cpp
        prototype.hpp
        registry.hpp
        script.cpp
        script.hpp
        value.cpp
        value.hpp
)

target_include_directories(graal-gs2 PUBLIC ${CMAKE_SOURCE_DIR}/src)
