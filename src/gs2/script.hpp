#pragma once

#include "environment.hpp"
#include "lexer.hpp"
#include "value.hpp"

#include <filesystem>
#include <istream>

namespace og::gs2 {
    struct script {
        virtual ~script() = default;

        [[nodiscard]]
        virtual auto has_function(std::string_view name) const -> bool = 0;

        [[nodiscard]]
        virtual auto call(std::string_view function_name, dictionary_ptr self, const values &args = {}) -> expected_value = 0;

        [[nodiscard]]
        virtual auto call_with(std::string_view function_name, dictionary_ptr self, dictionary_ptr locals, const values &args = {}) -> expected_value = 0;

        [[nodiscard]]
        virtual auto call_public(std::string_view function_name, dictionary_ptr self, const values &args = {}) -> expected_value = 0;

        [[nodiscard]]
        virtual auto is_public(std::string_view function_name) const -> bool = 0;

        virtual void set_name(std::string name) = 0;

        [[nodiscard]]
        virtual auto get_name() const -> const std::string & = 0;

        virtual auto join(const script_ptr &other) -> bool = 0;

        [[nodiscard]]
        virtual auto has_joined(std::string_view class_name) const -> bool = 0;
    };

    using script_result = std::expected<script_ptr, error>;

    auto load_script(environment &env, const tokens &tokens) -> script_result;
    auto load_script(environment &env, std::istream &is) -> script_result;

    auto compile(environment &env, const std::filesystem::path &path) -> script_result;

    struct script_halves {
        std::string server;
        std::string client;
    };

    [[nodiscard]] auto split_script(std::string_view source) -> script_halves;
}
