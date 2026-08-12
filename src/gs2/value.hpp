#pragma once

#include "error.hpp"

#include <expected>
#include <memory>
#include <variant>
#include <vector>

namespace og::gs2 {
    struct array;
    struct dictionary;
    struct callable;

    using array_ptr = std::shared_ptr<array>;
    using dictionary_ptr = std::shared_ptr<dictionary>;
    using callable_ptr = std::shared_ptr<callable>;

    using value = std::variant<std::monostate, double, std::string, array_ptr, dictionary_ptr, callable_ptr>;
    using values = std::vector<value>;

    auto to_number(const value &value) -> double;
    auto to_string(const value &value) -> std::string;
    auto to_bool(const value &value) -> bool;

    auto values_equal(const value &left, const value &right) -> bool;

    struct array {
        values elements;
    };

    struct dictionary : std::enable_shared_from_this<dictionary> {
        virtual ~dictionary() = default;

        virtual auto contains(std::string_view name) -> bool = 0;
        virtual auto get(std::string_view name) -> value = 0;
        virtual void put(std::string_view name, value val) = 0;
        virtual auto erase(std::string_view name) -> bool = 0;

        virtual auto keys() const -> std::vector<std::string> {
            return {};
        }
    };

    using expected_void = std::expected<void, error>;
    using expected_dictionary = std::expected<dictionary_ptr, error>;
    using expected_value = std::expected<value, error>;

    class context;

    struct callable {
        virtual ~callable() = default;

        virtual auto invoke(const values &args) -> expected_value = 0;
    };

    struct context_callable : callable {
        virtual auto invoke_with(context &ctx, const values &args) -> expected_value = 0;

        auto invoke(const values &args) -> expected_value override;
    };
}
