#pragma once

#include "environment.hpp"

#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace og::gs2 {
    inline constexpr size_t max_call_depth = 128;

    class context;

    using function_resolver = std::function<std::optional<value>(context &, std::string_view)>;

    class context {
      public:
        context(environment &env, dictionary_ptr self);
        context(environment &env, dictionary_ptr self, dictionary_ptr locals);

        [[nodiscard]]
        auto get_environment() const -> environment & {
            return env_;
        }

        [[nodiscard]]
        auto get_dictionary() const -> const dictionary_ptr & {
            return self_;
        }

        [[nodiscard]]
        auto get_origin() const -> const dictionary_ptr & {
            return origin_;
        }

        [[nodiscard]]
        auto get_locals() const -> const dictionary_ptr & {
            return locals_;
        }

        [[nodiscard]]
        auto get_temp() const -> const dictionary_ptr &;

        [[nodiscard]]
        auto get_params() const -> value;

        [[nodiscard]]
        auto get_depth() const -> size_t {
            return frames_.size();
        }

        [[nodiscard]]
        auto get_source_name() const -> const std::string & {
            return source_name_;
        }

        void set_source_name(std::string name) {
            source_name_ = std::move(name);
        }

        void bind_scope(std::string_view name, dictionary_ptr scope);

        [[nodiscard]]
        auto find_scope(std::string_view name) const -> dictionary_ptr;

        void set_script(script *value) {
            script_ = value;
        }

        [[nodiscard]]
        auto get_script() const -> script * {
            return script_;
        }

        void set_function_resolver(function_resolver resolver) {
            functions_ = std::move(resolver);
        }

        [[nodiscard]]
        auto resolve(std::string_view name) -> std::optional<value>;

        auto get(std::string_view name) -> value;

        class frame_scope {
          public:
            explicit frame_scope(context &owner) : owner_(&owner) {
            }

            frame_scope(const frame_scope &) = delete;
            auto operator=(const frame_scope &) -> frame_scope & = delete;
            frame_scope(frame_scope &&other) noexcept : owner_(std::exchange(other.owner_, nullptr)) {
            }
            auto operator=(frame_scope &&) -> frame_scope & = delete;

            ~frame_scope();

          private:
            context *owner_;
        };

        class self_scope {
          public:
            self_scope(context &owner, dictionary_ptr previous)
                : owner_(&owner), previous_(std::move(previous)) {
            }

            self_scope(const self_scope &) = delete;
            auto operator=(const self_scope &) -> self_scope & = delete;
            self_scope(self_scope &&other) noexcept
                : owner_(std::exchange(other.owner_, nullptr)), previous_(std::move(other.previous_)) {
            }
            auto operator=(self_scope &&) -> self_scope & = delete;

            ~self_scope();

          private:
            context *owner_;
            dictionary_ptr previous_;
        };

        class locals_scope {
          public:
            locals_scope(context &owner, dictionary_ptr previous)
                : owner_(&owner), previous_(std::move(previous)) {
            }

            locals_scope(const locals_scope &) = delete;
            auto operator=(const locals_scope &) -> locals_scope & = delete;
            locals_scope(locals_scope &&other) noexcept
                : owner_(std::exchange(other.owner_, nullptr)), previous_(std::move(other.previous_)) {
            }
            auto operator=(locals_scope &&) -> locals_scope & = delete;

            ~locals_scope();

          private:
            context *owner_;
            dictionary_ptr previous_;
        };

        class with_scope {
          public:
            explicit with_scope(context &owner) : owner_(&owner) {
            }

            with_scope(const with_scope &) = delete;
            auto operator=(const with_scope &) -> with_scope & = delete;
            with_scope(with_scope &&other) noexcept : owner_(std::exchange(other.owner_, nullptr)) {
            }
            auto operator=(with_scope &&) -> with_scope & = delete;

            ~with_scope();

          private:
            context *owner_;
        };

        [[nodiscard]]
        auto find_with(std::string_view name) const -> dictionary_ptr;

        [[nodiscard]] auto push_with(dictionary_ptr object) -> with_scope;

        [[nodiscard]] auto push_frame(dictionary_ptr temp, value params, std::vector<std::string> parameters = {}) -> frame_scope;

        [[nodiscard]] auto is_parameter(std::string_view name) const -> bool;
        [[nodiscard]] auto push_self(dictionary_ptr self) -> self_scope;
        [[nodiscard]] auto push_locals(dictionary_ptr locals) -> locals_scope;

      private:
        struct frame {
            dictionary_ptr temp;
            value params;
            std::vector<std::string> parameters;
        };

        environment &env_;
        dictionary_ptr self_;
        dictionary_ptr origin_;
        dictionary_ptr locals_;
        std::vector<dictionary_ptr> shadowed_locals_;
        std::string source_name_;
        std::vector<frame> frames_;
        std::vector<dictionary_ptr> with_;
        registry<dictionary_ptr> scopes_;
        function_resolver functions_;
        script *script_ = nullptr;
    };
}
