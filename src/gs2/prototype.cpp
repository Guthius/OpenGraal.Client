#include <gs2/prototype.hpp>

using namespace std;

namespace og::gs2 {
    namespace {
        struct prototype_callable : callable {
            void *instance;
            const function_binding *function;

            prototype_callable(void *instance, const function_binding *function)
                : instance(instance),
                  function(function) {
            }

            auto invoke(const values &args) -> expected_value override {
                return (*function)(instance, args);
            }
        };
    }

    auto prototype_dictionary::type_name() const -> string_view {
        return proto_->name;
    }

    auto prototype_dictionary::is_a(string_view name) const -> bool {
        return proto_->is_a(name);
    }

    auto prototype_dictionary::contains(string_view name) -> bool {
        return proto_->find_property(name) != nullptr ||
               proto_->find_function(name) != nullptr ||
               basic_dictionary::contains(name);
    }

    auto prototype_dictionary::get(string_view name) -> value {
        if (const auto *property = proto_->find_property(name)) {
            return property->getter(instance_.get());
        }

        if (const auto *function = proto_->find_function(name)) {
            return make_shared<prototype_callable>(instance_.get(), function);
        }

        return basic_dictionary::get(name);
    }

    void prototype_dictionary::put(string_view name, value value) {
        if (const auto *property = proto_->find_property(name)) {
            if (property->setter) {
                property->setter(instance_.get(), std::move(value));
            }

            return;
        }

        basic_dictionary::put(name, std::move(value));
    }

    auto prototype::find_property(string_view name) const -> const property_binding * {
        auto iter = properties.find(name);
        if (iter == properties.end()) {
            return nullptr;
        }

        return &iter->second;
    }

    auto prototype::find_function(string_view name) const -> const function_binding * {
        auto iter = functions.find(name);
        if (iter == functions.end()) {
            return nullptr;
        }

        return &iter->second;
    }

    auto prototype::is_a(string_view type_name) const -> bool {
        if (name == type_name) {
            return true;
        }

        return parent ? parent->is_a(type_name) : false;
    }
}
