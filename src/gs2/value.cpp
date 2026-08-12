#include <gs2/value.hpp>

#include <sstream>

using namespace std;

namespace og::gs2 {
    namespace {
        auto is_numeric(const string &text) -> bool {
            const auto *begin = text.c_str();
            char *end = nullptr;

            strtod(begin, &end);

            if (end == begin) {
                return text.find_first_not_of(" \t\r\n") == string::npos;
            }

            return string_view(end).find_first_not_of(" \t\r\n") == string_view::npos;
        }

        constexpr string generic_object_name = "Object";
    }

    auto to_number(const value &value) -> double {
        return visit(
            [](const auto &current) -> double {
                using T = decay_t<decltype(current)>;

                if constexpr (is_same_v<T, double>) {
                    return current;
                }

                if constexpr (is_same_v<T, string>) {
                    return strtod(current.c_str(), nullptr);
                }

                return 0.0;
            },
            value);
    }

    auto to_string(const value &value) -> string {
        return visit(
            [](const auto &current) -> string {
                using T = decay_t<decltype(current)>;

                if constexpr (is_same_v<T, monostate>) {
                    return "";
                }

                if constexpr (is_same_v<T, double>) {
                    ostringstream oss;
                    oss << current;
                    return oss.str();
                }

                if constexpr (is_same_v<T, string>) {
                    return current;
                }

                if constexpr (is_same_v<T, array_ptr>) {
                    if (!current) {
                        return "";
                    }

                    ostringstream oss;
                    for (size_t i = 0; i < current->elements.size(); ++i) {
                        if (i > 0) {
                            oss << ',';
                        }

                        oss << to_string(current->elements[i]);
                    }

                    return oss.str();
                }

                if constexpr (is_same_v<T, dictionary_ptr>) {
                    return generic_object_name;
                }

                return "";
            },
            value);
    }

    auto to_bool(const value &value) -> bool {
        return visit(
            [](const auto &current) -> bool {
                using T = decay_t<decltype(current)>;

                if constexpr (is_same_v<T, monostate>) {
                    return false;
                }

                if constexpr (is_same_v<T, double>) {
                    return current != 0.0;
                }

                if constexpr (is_same_v<T, string>) {
                    return !current.empty();
                }

                if constexpr (is_same_v<T, array_ptr>) {
                    return current != nullptr;
                }

                if constexpr (is_same_v<T, dictionary_ptr>) {
                    return current != nullptr;
                }

                return false;
            },
            value);
    }

    auto values_equal(const value &left, const value &right) -> bool {
        // null == null
        if (holds_alternative<monostate>(left) &&
            holds_alternative<monostate>(right)) {
            return true;
        }

        // One side is null
        if (holds_alternative<monostate>(left) ||
            holds_alternative<monostate>(right)) {
            const auto &other = holds_alternative<monostate>(left) ? right : left;

            if (const auto *text = get_if<string>(&other)) {
                return text->empty();
            }

            if (const auto *number = get_if<double>(&other)) {
                return *number == 0.0;
            }

            return false;
        }

        // Both numeric
        if (holds_alternative<double>(left) &&
            holds_alternative<double>(right)) {
            return get<double>(left) == get<double>(right);
        }

        // Both string
        if (holds_alternative<string>(left) &&
            holds_alternative<string>(right)) {
            return get<string>(left) == get<string>(right);
        }

        if ((holds_alternative<double>(left) || holds_alternative<string>(left)) &&
            (holds_alternative<double>(right) || holds_alternative<string>(right))) {
            const auto *text = get_if<string>(&left);
            if (text == nullptr) {
                text = get_if<string>(&right);
            }

            if (text != nullptr && !is_numeric(*text)) {
                return false;
            }

            return to_number(left) == to_number(right);
        }

        // Object identity
        if (holds_alternative<dictionary_ptr>(left) && holds_alternative<dictionary_ptr>(right)) {
            return get<dictionary_ptr>(left) == get<dictionary_ptr>(right);
        }

        if (holds_alternative<array_ptr>(left) && holds_alternative<array_ptr>(right)) {
            return get<array_ptr>(left) == get<array_ptr>(right);
        }

        return false;
    }
}

namespace og::gs2 {
    auto context_callable::invoke(const values &args) -> expected_value {
        return unexpected(error{
            .source = error_source::interpreter,
            .kind = error_kind::runtime_error,
            .message = "this function must be called from a script",
        });
    }
}
