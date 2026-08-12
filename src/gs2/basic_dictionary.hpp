#pragma once

#include "registry.hpp"
#include "value.hpp"

#include <ranges>
#include <vector>

namespace og::gs2 {
    template <typename TRegistry>
    class dictionary_base : public dictionary {
      public:
        auto contains(std::string_view name) -> bool override {
            return fields_.contains(name);
        }

        auto get(std::string_view name) -> value override {
            auto iter = fields_.find(name);
            if (iter == fields_.end()) {
                return {};
            }

            return iter->second;
        }

        void put(std::string_view name, value value) override {
            fields_[std::string(name)] = std::move(value);
        }

        auto erase(std::string_view name) -> bool override {
            auto iter = fields_.find(name);
            if (iter == fields_.end()) {
                return false;
            }

            fields_.erase(iter);

            return true;
        }

        auto keys() const -> std::vector<std::string> override {
            std::vector<std::string> names;
            names.reserve(fields_.size());

            for (const auto &name : fields_ | std::views::keys) {
                names.push_back(name);
            }

            return names;
        }

      private:
        TRegistry fields_;
    };

    using basic_dictionary = dictionary_base<registry<value>>;
    using folded_dictionary = dictionary_base<folded_registry<value>>;
}
