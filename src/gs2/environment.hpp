#pragma once

#include "prototype.hpp"

#include <functional>
#include <optional>
#include <string>

namespace og::gs2 {
    class environment;
    struct script;

    using script_ptr = std::shared_ptr<script>;
    using class_resolver = std::function<std::optional<std::string>(std::string_view name)>;
    using file_resolver = std::function<std::optional<std::string>(std::string_view name)>;

    using folder_resolver = std::function<std::vector<std::string>(std::string_view pattern, bool recursive)>;
    using file_writer = std::function<bool(std::string_view name, std::string_view contents)>;

    using object_sink = std::function<void(const dictionary_ptr &object, const dictionary_ptr &enclosing)>;

    using native_function = std::function<expected_value(environment &env, const values &args)>;

    class environment : public folded_dictionary {
      public:
        environment();

        void register_function(std::string_view name, const native_function &function);
        void register_type(prototype_ptr prototype);
        auto find_type(std::string_view type_name) -> prototype_ptr;

        void set_class_resolver(class_resolver resolver) {
            class_resolver_ = std::move(resolver);
        }

        auto find_class(std::string_view class_name) -> script_ptr;

        void set_folder_resolver(folder_resolver resolver) {
            folder_resolver_ = std::move(resolver);
        }

        [[nodiscard]] auto list_folder(std::string_view pattern, const bool recursive) const -> std::vector<std::string> {
            return folder_resolver_ ? folder_resolver_(pattern, recursive) : std::vector<std::string>{};
        }

        void set_file_resolver(file_resolver resolver) {
            file_resolver_ = std::move(resolver);
        }

        [[nodiscard]] auto read_file(std::string_view name) const -> std::optional<std::string> {
            return file_resolver_ ? file_resolver_(name) : std::nullopt;
        }

        void set_file_writer(file_writer writer) {
            file_writer_ = std::move(writer);
        }

        [[nodiscard]] auto write_file(std::string_view name, std::string_view contents) const -> bool {
            return file_writer_ && file_writer_(name, contents);
        }

        void set_object_sink(object_sink sink) {
            object_sink_ = std::move(sink);
        }

        void object_created(const dictionary_ptr &object, const dictionary_ptr &enclosing) const {
            if (object_sink_) {
                object_sink_(object, enclosing);
            }
        }

        void register_object(std::string_view name, dictionary_ptr object);

        [[nodiscard]] auto find_object(std::string_view name) const -> dictionary_ptr;
        auto erase_object(std::string_view name) -> bool;
        void clear_objects();

      private:
        registry<prototype_ptr> types_;
        registry<script_ptr> classes_;
        folded_registry<dictionary_ptr> objects_;
        class_resolver class_resolver_;
        file_resolver file_resolver_;
        folder_resolver folder_resolver_;
        file_writer file_writer_;
        object_sink object_sink_;
    };
}
