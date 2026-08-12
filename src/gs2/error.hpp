#pragma once

#include <cstdint>
#include <string>

namespace og::gs2 {
    struct source_position {
        int line, column;

        static constexpr auto none() {
            return source_position{
                .line = 0,
                .column = 0,
            };
        }
    };

    enum class error_source : uint8_t {
        lexer,
        parser,
        interpreter,
    };

    enum class error_kind : uint8_t {
        stream_error,
        token_error,
        syntax_error,
        file_error,
        runtime_error,
        halted,
    };

    struct error {
        error_source source;
        error_kind kind;
        std::string message;
        source_position position = source_position::none();
        std::string source_name;
        std::string function_name;
    };
}
