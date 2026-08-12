#include <gs2/context.hpp>

#include <ranges>

using namespace std;

namespace og::gs2 {
    namespace {
        const dictionary_ptr no_dictionary;
    }

    context::context(environment &env, dictionary_ptr self)
        : context(env, std::move(self), make_shared<basic_dictionary>()) {
    }

    context::context(environment &env, dictionary_ptr self, dictionary_ptr locals)
        : env_(env),
          self_(std::move(self)),
          origin_(self_),
          locals_(std::move(locals)) {
        if (!locals_) {
            locals_ = make_shared<basic_dictionary>();
        }
    }

    context::frame_scope::~frame_scope() {
        if (owner_ != nullptr) {
            owner_->frames_.pop_back();
        }
    }

    context::self_scope::~self_scope() {
        if (owner_ != nullptr) {
            owner_->self_ = std::move(previous_);
        }
    }

    context::locals_scope::~locals_scope() {
        if (owner_ != nullptr) {
            owner_->shadowed_locals_.pop_back();
            owner_->locals_ = std::move(previous_);
        }
    }

    context::with_scope::~with_scope() {
        if (owner_ != nullptr) {
            owner_->with_.pop_back();
        }
    }

    auto context::push_with(dictionary_ptr object) -> with_scope {
        with_.push_back(std::move(object));

        return with_scope(*this);
    }

    auto context::find_with(const string_view name) const -> dictionary_ptr {
        for (const auto &object : ranges::reverse_view(with_)) {
            if (object && object->contains(name)) {
                return object;
            }
        }

        return nullptr;
    }

    auto context::push_locals(dictionary_ptr locals) -> locals_scope {
        auto previous = std::exchange(locals_, std::move(locals));
        shadowed_locals_.push_back(previous);

        return {*this, std::move(previous)};
    }

    auto context::push_frame(dictionary_ptr temp, value params, vector<string> parameters) -> frame_scope {
        frames_.push_back(frame{
            .temp = std::move(temp),
            .params = std::move(params),
            .parameters = std::move(parameters),
        });

        return frame_scope(*this);
    }

    auto context::is_parameter(const string_view name) const -> bool {
        return !frames_.empty() && ranges::contains(frames_.back().parameters, name);
    }

    auto context::push_self(dictionary_ptr self) -> self_scope {
        auto previous = std::exchange(self_, std::move(self));

        return {*this, std::move(previous)};
    }

    auto context::get_temp() const -> const dictionary_ptr & {
        return frames_.empty() ? no_dictionary : frames_.back().temp;
    }

    auto context::get_params() const -> value {
        return frames_.empty() ? value{} : frames_.back().params;
    }

    void context::bind_scope(const string_view name, dictionary_ptr scope) {
        scopes_[string(name)] = std::move(scope);
    }

    auto context::find_scope(const string_view name) const -> dictionary_ptr {
        const auto match = scopes_.find(name);

        return match == scopes_.end() ? nullptr : match->second;
    }

    auto context::resolve(const string_view name) -> optional<value> {
        if (name == "this") {
            return self_ ? value{self_} : value{};
        }

        if (name == "thiso") {
            return origin_ ? value{origin_} : value{};
        }

        if (name == "temp") {
            const auto &temp = get_temp();

            return temp ? value{temp} : value{};
        }

        if (name == "params") {
            return get_params();
        }

        if (is_parameter(name)) {
            return get_temp()->get(name);
        }

        if (auto scope = find_scope(name)) {
            return value{std::move(scope)};
        }

        if (auto object = find_with(name)) {
            return object->get(name);
        }

        if (locals_->contains(name)) {
            return locals_->get(name);
        }

        for (const auto &shadowed : ranges::reverse_view(shadowed_locals_)) {
            if (shadowed && shadowed->contains(name)) {
                return shadowed->get(name);
            }
        }

        if (functions_) {
            if (auto function = functions_(*this, name)) {
                return function;
            }
        }

        if (auto object = env_.find_object(name)) {
            return value{std::move(object)};
        }

        if (env_.contains(name)) {
            return env_.get(name);
        }

        return nullopt;
    }

    auto context::get(const string_view name) -> value {
        return resolve(name).value_or(value{});
    }
}
