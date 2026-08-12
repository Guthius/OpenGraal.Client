#include <gs2/environment.hpp>

#include <gs2/builtins.hpp>
#include <gs2/prototype.hpp>
#include <gs2/script.hpp>

#include <sstream>

using namespace std;

namespace og::gs2 {
    namespace {
        struct callable_impl : callable {
            environment &env;
            native_function function;

            callable_impl(environment &env, native_function function)
                : env(env), function(std::move(function)) {
            }

            auto invoke(const values &args) -> expected_value override {
                return function(env, args);
            }
        };
    }

    environment::environment() {
        register_standard_functions(*this);
    }

    auto environment::find_class(const std::string_view class_name) -> script_ptr {
        if (const auto cached = classes_.find(class_name); cached != classes_.end()) {
            return cached->second;
        }

        if (!class_resolver_) {
            return nullptr;
        }

        auto source = class_resolver_(class_name);
        if (!source) {
            return nullptr;
        }

        istringstream is(*source);

        auto script = load_script(*this, is);
        if (!script) {
            return nullptr;
        }

        (*script)->set_name(string(class_name));

        classes_[string(class_name)] = *script;

        return *script;
    }

    void environment::register_function(std::string_view name, const native_function &function) {
        put(name, std::make_shared<callable_impl>(*this, function));
    }

    void environment::register_type(prototype_ptr prototype) {
        types_[prototype->name] = std::move(prototype);
    }

    void environment::register_object(const std::string_view name, dictionary_ptr object) {
        if (name.empty() || !object) {
            return;
        }

        objects_[string(name)] = std::move(object);
    }

    auto environment::find_object(const std::string_view name) const -> dictionary_ptr {
        const auto match = objects_.find(name);

        return match == objects_.end() ? nullptr : match->second;
    }

    auto environment::erase_object(const std::string_view name) -> bool {
        const auto match = objects_.find(name);
        if (match == objects_.end()) {
            return false;
        }

        objects_.erase(match);

        return true;
    }

    void environment::clear_objects() {
        objects_.clear();
    }

    auto environment::find_type(std::string_view type_name) -> prototype_ptr {
        auto iter = types_.find(type_name);
        if (iter == types_.end()) {
            return nullptr;
        }

        return iter->second;
    }
}
