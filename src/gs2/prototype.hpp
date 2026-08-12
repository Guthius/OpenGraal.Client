#pragma once

#include "basic_dictionary.hpp"

#include <functional>
#include <string>
#include <type_traits>

namespace og::gs2 {
    struct prototype;

    using prototype_ptr = std::shared_ptr<prototype>;

    struct property_binding {
        std::function<value(void *instance)> getter;
        std::function<void(void *instance, value val)> setter;
    };

    using property_bindings = folded_registry<property_binding>;

    using function_binding = std::function<expected_value(void *instance, const values &args)>;
    using function_bindings = folded_registry<function_binding>;

    class prototype_dictionary : public basic_dictionary {
      public:
        prototype_dictionary(prototype_ptr proto, std::shared_ptr<void> instance)
            : proto_(std::move(proto)),
              instance_(std::move(instance)) {
        }

        [[nodiscard]]
        auto type_name() const -> std::string_view;

        [[nodiscard]]
        auto is_a(std::string_view name) const -> bool;

        auto contains(std::string_view name) -> bool override;
        auto get(std::string_view name) -> value override;
        void put(std::string_view name, value value) override;

        [[nodiscard]]
        auto get_instance() const -> const std::shared_ptr<void> & {
            return instance_;
        }

      private:
        prototype_ptr proto_;
        std::shared_ptr<void> instance_;
    };

    using factory_binding = std::function<std::shared_ptr<void>(const values &args)>;

    struct prototype {
        std::string name;
        prototype_ptr parent;
        property_bindings properties;
        function_bindings functions;
        factory_binding factory;

        auto find_property(std::string_view name) const -> const property_binding *;
        auto find_function(std::string_view name) const -> const function_binding *;
        auto is_a(std::string_view type_name) const -> bool;
    };

    template <typename TSignature>
    struct setter_traits;

    template <typename TClass, typename TInstance, typename TValue>
    struct setter_traits<void (TClass::*)(TInstance, TValue) const> {
        using value_type = std::decay_t<TValue>;
    };

    template <typename TClass, typename TInstance, typename TValue>
    struct setter_traits<void (TClass::*)(TInstance, TValue)> {
        using value_type = std::decay_t<TValue>;
    };

    template <typename TSetter>
    using setter_value_t = typename setter_traits<decltype(&TSetter::operator())>::value_type;

    template <typename T>
    class prototype_builder {
      public:
        explicit prototype_builder(std::string name, prototype_ptr parent = nullptr) : proto_(std::make_shared<prototype>()) {
            proto_->name = std::move(name);
            proto_->parent = std::move(parent);
        }

        template <typename TGetter, typename TSetter = std::nullptr_t>
        auto property(std::string_view name, TGetter getter, TSetter setter = nullptr) -> prototype_builder & {
            auto binding = property_binding{
                .getter = [getter](void *instance) -> value {
                    return to_value(getter(static_cast<T *>(instance)));
                },
            };

            if constexpr (!std::is_null_pointer_v<TSetter>) {
                binding.setter = [setter](void *instance, value new_value) -> void {
                    setter(static_cast<T *>(instance), from_value<setter_value_t<TSetter>>(new_value));
                };
            }

            proto_->properties[std::string(name)] = std::move(binding);

            return *this;
        }

        template <typename TFactory>
        auto constructor(TFactory factory) -> prototype_builder & {
            proto_->factory = [factory](const values &args) -> std::shared_ptr<void> {
                return factory(args);
            };

            return *this;
        }

        auto constructor() -> prototype_builder & {
            proto_->factory = [](const values &) -> std::shared_ptr<void> {
                return std::make_shared<T>();
            };

            return *this;
        }

        template <typename TFunction>
        auto function(std::string_view name, TFunction function) -> prototype_builder & {
            proto_->functions[std::string(name)] = [function](void *instance, const values &args) -> expected_value {
                return function(static_cast<T *>(instance), args);
            };

            return *this;
        }

        [[nodiscard]]
        auto build() -> prototype_ptr {
            return proto_;
        }

      private:
        prototype_ptr proto_;

        template <typename V>
        static auto to_value(V val) -> value {
            using U = std::decay_t<V>;

            if constexpr (std::is_same_v<U, bool>)
                return value{val ? 1.0 : 0.0};
            else if constexpr (std::is_arithmetic_v<U>)
                return value{static_cast<double>(val)};
            else if constexpr (std::is_same_v<U, std::string>)
                return value{val};
            else if constexpr (std::is_same_v<U, std::string_view>)
                return value{std::string(val)};
            else
                return value{};
        }

        template <typename V>
        static auto from_value(const value &value) -> V {
            if constexpr (std::is_same_v<V, bool>)
                return to_bool(value);
            else if constexpr (std::is_arithmetic_v<V>)
                return static_cast<V>(to_number(value));
            else if constexpr (std::is_same_v<V, std::string>)
                return to_string(value);
            else
                return V{};
        }
    };
}
