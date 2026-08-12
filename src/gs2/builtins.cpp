#include <gs2/builtins.hpp>

#include <gs2/context.hpp>
#include <gs2/script.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <sstream>

using namespace std;

namespace og::gs2 {
    namespace {
        constexpr string_view whitespace = " \t\r\n\v\f";
        constexpr double pi = 3.14159265358979323846;

        auto arg(const values &args, const size_t index) -> value {
            return index < args.size() ? args[index] : value{};
        }

        auto arg_number(const values &args, const size_t index, const double fallback = 0.0) -> double {
            return index < args.size() ? to_number(args[index]) : fallback;
        }

        auto arg_index(const values &args, const size_t index) -> size_t {
            const auto raw = arg_number(args, index);

            return raw <= 0.0 ? 0 : static_cast<size_t>(raw);
        }

        auto value_type_id(const value &val) -> double {
            if (holds_alternative<string>(val)) return 1.0;
            if (holds_alternative<dictionary_ptr>(val) || holds_alternative<callable_ptr>(val)) return 2.0;
            if (holds_alternative<array_ptr>(val)) return 3.0;

            return 0.0;
        }

        auto trim_copy(const string_view str) -> string {
            const auto first = str.find_first_not_of(whitespace);
            if (first == string_view::npos) {
                return {};
            }

            const auto last = str.find_last_not_of(whitespace);

            return string(str.substr(first, last - first + 1));
        }

        auto tokenize_string(const string_view str, const string_view delimiters) -> array_ptr {
            auto tokens = make_shared<array>();

            size_t start = 0;
            while (start < str.size()) {
                start = str.find_first_not_of(delimiters, start);
                if (start == string_view::npos) {
                    break;
                }

                const auto end = str.find_first_of(delimiters, start);
                const auto token = end == string_view::npos ? str.substr(start) : str.substr(start, end - start);

                tokens->elements.emplace_back(string(token));

                if (end == string_view::npos) {
                    break;
                }

                start = end;
            }

            return tokens;
        }

        struct native_method : callable {
            using body = function<expected_value(const value &target, const values &args)>;

            value target;
            body invoke_body;

            native_method(value target, body invoke_body)
                : target(std::move(target)), invoke_body(std::move(invoke_body)) {
            }

            auto invoke(const values &args) -> expected_value override {
                return invoke_body(target, args);
            }
        };

        struct native_context_function : context_callable {
            using body = function<expected_value(context &ctx, const values &args)>;

            body invoke_body;

            explicit native_context_function(body invoke_body) : invoke_body(std::move(invoke_body)) {
            }

            auto invoke_with(context &ctx, const values &args) -> expected_value override {
                return invoke_body(ctx, args);
            }
        };

        auto make_method(const value &target, native_method::body body) -> callable_ptr {
            return make_shared<native_method>(target, std::move(body));
        }

        auto find_string_method(const value &target, const string_view name) -> callable_ptr {
            if (name == "length") {
                return make_method(target, [](const value &self, const values &) -> expected_value {
                    return value{static_cast<double>(to_string(self).size())};
                });
            }

            if (name == "trim") {
                return make_method(target, [](const value &self, const values &) -> expected_value {
                    return value{trim_copy(to_string(self))};
                });
            }

            if (name == "starts") {
                return make_method(target, [](const value &self, const values &args) -> expected_value {
                    return to_string(self).starts_with(to_string(arg(args, 0))) ? value{1.0} : value{0.0};
                });
            }

            if (name == "ends") {
                return make_method(target, [](const value &self, const values &args) -> expected_value {
                    return to_string(self).ends_with(to_string(arg(args, 0))) ? value{1.0} : value{0.0};
                });
            }

            if (name == "pos") {
                return make_method(target, [](const value &self, const values &args) -> expected_value {
                    const auto found = to_string(self).find(to_string(arg(args, 0)));

                    return value{found == string::npos ? -1.0 : static_cast<double>(found)};
                });
            }

            if (name == "charat") {
                return make_method(target, [](const value &self, const values &args) -> expected_value {
                    const auto text = to_string(self);
                    const auto index = arg_index(args, 0);

                    return value{index < text.size() ? string(1, text[index]) : string{}};
                });
            }

            if (name == "substring") {
                return make_method(target, [](const value &self, const values &args) -> expected_value {
                    const auto text = to_string(self);
                    const auto start = arg_index(args, 0);

                    if (start >= text.size()) {
                        return value{string{}};
                    }

                    if (args.size() < 2) {
                        return value{text.substr(start)};
                    }

                    const auto length = arg_number(args, 1);
                    if (length < 0.0) {
                        return value{text.substr(start)};
                    }

                    if (length == 0.0) {
                        return value{string{}};
                    }

                    return value{text.substr(start, static_cast<size_t>(length))};
                });
            }

            if (name == "tokenize") {
                return make_method(target, [](const value &self, const values &args) -> expected_value {
                    const auto delimiters = args.empty() ? string(" ") : to_string(args[0]);

                    return value{tokenize_string(to_string(self), delimiters.empty() ? " " : delimiters)};
                });
            }

            return nullptr;
        }

        auto find_array_method(environment &env, const value &target, const string_view name) -> callable_ptr {
            const auto elements_of = [](const value &self) -> array_ptr {
                const auto *elements = get_if<array_ptr>(&self);

                return elements == nullptr ? nullptr : *elements;
            };

            if (name == "size" || name == "length") {
                return make_method(target, [elements_of](const value &self, const values &) -> expected_value {
                    const auto elements = elements_of(self);

                    return value{elements ? static_cast<double>(elements->elements.size()) : 0.0};
                });
            }

            if (name == "loadfolder") {
                return make_method(target, [elements_of, &env](const value &self, const values &args) -> expected_value {
                    const auto elements = elements_of(self);
                    if (!elements) {
                        return value{0.0};
                    }

                    elements->elements.clear();

                    for (auto &found : env.list_folder(to_string(arg(args, 0)), to_bool(arg(args, 1)))) {
                        elements->elements.emplace_back(std::move(found));
                    }

                    return value{static_cast<double>(elements->elements.size())};
                });
            }

            if (name == "add") {
                return make_method(target, [elements_of](const value &self, const values &args) -> expected_value {
                    if (const auto elements = elements_of(self)) {
                        elements->elements.push_back(arg(args, 0));
                    }

                    return self;
                });
            }

            if (name == "addarray") {
                return make_method(target, [elements_of](const value &self, const values &args) -> expected_value {
                    const auto elements = elements_of(self);
                    const auto *other = args.empty() ? nullptr : get_if<array_ptr>(args.data());

                    if (elements && other != nullptr && *other) {
                        const auto &source = (*other)->elements;
                        elements->elements.insert(elements->elements.end(), source.begin(), source.end());
                    }

                    return self;
                });
            }

            if (name == "index") {
                return make_method(target, [elements_of](const value &self, const values &args) -> expected_value {
                    const auto elements = elements_of(self);
                    if (!elements) {
                        return value{-1.0};
                    }

                    const auto needle = arg(args, 0);
                    for (size_t i = 0; i < elements->elements.size(); ++i) {
                        if (values_equal(elements->elements[i], needle)) {
                            return value{static_cast<double>(i)};
                        }
                    }

                    return value{-1.0};
                });
            }

            if (name == "remove") {
                return make_method(target, [elements_of](const value &self, const values &args) -> expected_value {
                    const auto elements = elements_of(self);
                    if (!elements) {
                        return self;
                    }

                    const auto needle = arg(args, 0);
                    const auto match = ranges::find_if(elements->elements, [&needle](const value &item) {
                        return values_equal(item, needle);
                    });

                    if (match != elements->elements.end()) {
                        elements->elements.erase(match);
                    }

                    return self;
                });
            }

            if (name == "delete") {
                return make_method(target, [elements_of](const value &self, const values &args) -> expected_value {
                    const auto elements = elements_of(self);
                    const auto index = arg_index(args, 0);

                    if (elements && index < elements->elements.size()) {
                        elements->elements.erase(elements->elements.begin() + static_cast<ptrdiff_t>(index));
                    }

                    return self;
                });
            }

            if (name == "insert") {
                return make_method(target, [elements_of](const value &self, const values &args) -> expected_value {
                    const auto elements = elements_of(self);
                    if (!elements) {
                        return self;
                    }

                    const auto index = min(arg_index(args, 0), elements->elements.size());
                    elements->elements.insert(elements->elements.begin() + static_cast<ptrdiff_t>(index), arg(args, 1));

                    return self;
                });
            }

            if (name == "insertarray") {
                return make_method(target, [elements_of](const value &self, const values &args) -> expected_value {
                    const auto elements = elements_of(self);
                    const auto *other = args.size() < 2 ? nullptr : get_if<array_ptr>(&args[1]);

                    if (elements && other != nullptr && *other) {
                        const auto index = min(arg_index(args, 0), elements->elements.size());
                        const auto &source = (*other)->elements;

                        elements->elements.insert(
                            elements->elements.begin() + static_cast<ptrdiff_t>(index),
                            source.begin(),
                            source.end());
                    }

                    return self;
                });
            }

            if (name == "replace") {
                return make_method(target, [elements_of](const value &self, const values &args) -> expected_value {
                    const auto elements = elements_of(self);
                    const auto index = arg_index(args, 0);

                    if (elements && index < elements->elements.size()) {
                        elements->elements[index] = arg(args, 1);
                    }

                    return self;
                });
            }

            if (name == "clear") {
                return make_method(target, [elements_of](const value &self, const values &) -> expected_value {
                    if (const auto elements = elements_of(self)) {
                        elements->elements.clear();
                    }

                    return self;
                });
            }

            if (name == "subarray") {
                return make_method(target, [elements_of](const value &self, const values &args) -> expected_value {
                    auto result = make_shared<array>();

                    const auto elements = elements_of(self);
                    if (!elements) {
                        return value{result};
                    }

                    const auto start = arg_index(args, 0);
                    if (start >= elements->elements.size()) {
                        return value{result};
                    }

                    const auto available = elements->elements.size() - start;
                    const auto count = args.size() < 2
                                           ? available
                                           : min(available, static_cast<size_t>(max(0.0, arg_number(args, 1))));

                    result->elements.assign(
                        elements->elements.begin() + static_cast<ptrdiff_t>(start),
                        elements->elements.begin() + static_cast<ptrdiff_t>(start + count));

                    return value{result};
                });
            }

            return nullptr;
        }
    }

    namespace {
        void put_nested(dictionary &target, const string_view path, string text) {
            const auto dot = path.find('.');
            if (dot == string_view::npos) {
                target.put(path, value{std::move(text)});

                return;
            }

            const auto head = path.substr(0, dot);

            auto child = target.get(head);
            auto *nested = get_if<dictionary_ptr>(&child);

            if (nested == nullptr || !*nested) {
                auto created = make_shared<basic_dictionary>();
                target.put(head, value{created});

                put_nested(*created, path.substr(dot + 1), std::move(text));

                return;
            }

            put_nested(**nested, path.substr(dot + 1), std::move(text));
        }

        void read_vars(dictionary &target, const string_view source, const string_view separators = "\n") {
            size_t start = 0;

            while (start <= source.size()) {
                const auto end = source.find_first_of(separators, start);
                const auto line = end == string_view::npos ? source.substr(start) : source.substr(start, end - start);

                if (const auto split = line.find('='); split != string_view::npos) {
                    if (const auto name = trim_copy(line.substr(0, split)); !name.empty()) {
                        put_nested(target, name, trim_copy(line.substr(split + 1)));
                    }
                }

                if (end == string_view::npos) {
                    break;
                }

                start = end + 1;
            }
        }

        void write_field(ostringstream &oss, const string &prefix, const value &field);

        void write_nested(ostringstream &oss, const string &prefix, const dictionary_ptr &object) {
            auto names = object->keys();
            ranges::sort(names);

            for (const auto &name : names) {
                write_field(oss, prefix.empty() ? name : prefix + "." + name, object->get(name));
            }
        }

        void write_field(ostringstream &oss, const string &prefix, const value &field) {
            if (const auto *nested = get_if<dictionary_ptr>(&field); nested != nullptr && *nested) {
                write_nested(oss, prefix, *nested);

                return;
            }

            oss << prefix << '=';

            if (const auto *elements = get_if<array_ptr>(&field); elements != nullptr && *elements) {
                auto first = true;
                for (const auto &element : (*elements)->elements) {
                    if (!exchange(first, false)) {
                        oss << ',';
                    }

                    oss << to_string(element);
                }
            } else {
                oss << to_string(field);
            }

            oss << '\n';
        }

        auto write_vars(const dictionary_ptr &object) -> string {
            ostringstream oss;

            write_nested(oss, {}, object);

            return oss.str();
        }

        auto find_object_method(environment &env, const value &target, const string_view name) -> callable_ptr {
            if (name == "loadvars") {
                return make_method(target, [&env](const value &self, const values &args) -> expected_value {
                    const auto *object = get_if<dictionary_ptr>(&self);
                    if (object == nullptr || !*object) {
                        return value{0.0};
                    }

                    auto source = env.read_file(to_string(arg(args, 0)));
                    if (!source) {
                        return value{0.0};
                    }

                    read_vars(**object, *source);

                    return value{1.0};
                });
            }

            if (name == "loadvarsfromarray") {
                return make_method(target, [](const value &self, const values &args) -> expected_value {
                    const auto *object = get_if<dictionary_ptr>(&self);
                    if (object == nullptr || !*object) {
                        return value{0.0};
                    }

                    for (const auto &name : (*object)->keys()) {
                        (*object)->erase(name);
                    }

                    const auto source = arg(args, 0);
                    const auto *lines = get_if<array_ptr>(&source);

                    if (lines == nullptr || !*lines) {
                        read_vars(**object, to_string(source), "\n,");

                        return value{1.0};
                    }

                    for (const auto &line : (*lines)->elements) {
                        read_vars(**object, to_string(line));
                    }

                    return value{1.0};
                });
            }

            if (name == "savevars") {
                return make_method(target, [&env](const value &self, const values &args) -> expected_value {
                    const auto *object = get_if<dictionary_ptr>(&self);
                    if (object == nullptr || !*object) {
                        return value{0.0};
                    }

                    return value{env.write_file(to_string(arg(args, 0)), write_vars(*object)) ? 1.0 : 0.0};
                });
            }

            if (name == "getdynamicvarnames") {
                return make_method(target, [](const value &self, const values &) -> expected_value {
                    auto names = make_shared<array>();

                    if (const auto *object = get_if<dictionary_ptr>(&self); object != nullptr && *object) {
                        for (auto &name : (*object)->keys()) {
                            names->elements.emplace_back(std::move(name));
                        }
                    }

                    return value{names};
                });
            }

            return nullptr;
        }
    }

    auto resolve_path(context &ctx, const string_view path, const bool create) -> value {
        auto current = value{};
        dictionary_ptr owner;
        size_t start = 0;
        auto first = true;

        while (start <= path.size()) {
            const auto dot = path.find('.', start);
            const auto part = dot == string_view::npos ? path.substr(start) : path.substr(start, dot - start);

            if (part.empty()) {
                return {};
            }

            if (first) {
                owner = ctx.find_scope(part);
                current = ctx.get(part);
                first = false;
            } else if (const auto *dictionary = get_if<dictionary_ptr>(&current); dictionary != nullptr && *dictionary) {
                owner = *dictionary;
                current = owner->get(part);
            } else {
                return {};
            }

            if (create && owner && holds_alternative<monostate>(current)) {
                current = value{make_shared<basic_dictionary>()};
                owner->put(part, current);
            }

            if (dot == string_view::npos) {
                break;
            }

            start = dot + 1;
        }

        return current;
    }

    auto find_method(environment &env, const value &target, const string_view name) -> callable_ptr {
        const auto folded = fold_case(name);

        if (folded == "type") {
            return make_method(target, [](const value &self, const values &) -> expected_value {
                return value{value_type_id(self)};
            });
        }

        if (holds_alternative<array_ptr>(target)) {
            return find_array_method(env, target, folded);
        }

        if (holds_alternative<dictionary_ptr>(target)) {
            return find_object_method(env, target, folded);
        }

        if (holds_alternative<callable_ptr>(target)) {
            return nullptr;
        }

        return find_string_method(target, folded);
    }

    auto format_string(const string_view format, const values &args, const size_t first_arg) -> string {
        ostringstream oss;

        auto next = first_arg;
        for (size_t i = 0; i < format.size(); ++i) {
            if (format[i] != '%' || i + 1 >= format.size()) {
                oss << format[i];
                continue;
            }

            const auto specifier = format[++i];
            switch (specifier) {
            case '%':
                oss << '%';
                break;

            case 's':
                oss << to_string(arg(args, next++));
                break;

            case 'd':
            case 'i':
                oss << static_cast<long long>(trunc(to_number(arg(args, next++))));
                break;

            case 'f':
            case 'g':
                oss << to_number(arg(args, next++));
                break;

            default:
                oss << '%' << specifier;
                break;
            }
        }

        return oss.str();
    }

    void register_standard_functions(environment &env) {
        const auto simple = [&env](const string_view name, function<double(const values &)> body) {
            env.register_function(name, [body = std::move(body)](environment &, const values &args) -> expected_value {
                return value{body(args)};
            });
        };

        simple("int", [](const values &args) { return trunc(arg_number(args, 0)); });
        simple("abs", [](const values &args) { return fabs(arg_number(args, 0)); });
        simple("sin", [](const values &args) { return sin(arg_number(args, 0)); });
        simple("cos", [](const values &args) { return cos(arg_number(args, 0)); });
        simple("arctan", [](const values &args) { return atan(arg_number(args, 0)); });
        simple("exp", [](const values &args) { return exp(arg_number(args, 0)); });
        simple("min", [](const values &args) { return min(arg_number(args, 0), arg_number(args, 1)); });
        simple("max", [](const values &args) { return max(arg_number(args, 0), arg_number(args, 1)); });

        simple("log", [](const values &args) {
            const auto base = arg_number(args, 0);
            const auto operand = arg_number(args, 1);

            if (base <= 0.0 || base == 1.0 || operand <= 0.0) {
                return 0.0;
            }

            return std::log(operand) / std::log(base);
        });

        simple("random", [](const values &args) {
            static thread_local mt19937 engine{random_device{}()};

            const auto low = arg_number(args, 0);
            const auto high = arg_number(args, 1);

            if (high <= low) {
                return low;
            }

            return uniform_real_distribution(low, high)(engine);
        });

        simple("vecx", [](const values &args) {
            constexpr std::array directions = {0.0, -1.0, 0.0, 1.0};

            const auto index = static_cast<size_t>(fmod(max(0.0, arg_number(args, 0)), 4.0));

            return directions[index];
        });

        simple("vecy", [](const values &args) {
            constexpr std::array directions = {-1.0, 0.0, 1.0, 0.0};

            const auto index = static_cast<size_t>(fmod(max(0.0, arg_number(args, 0)), 4.0));

            return directions[index];
        });

        simple("getangle", [](const values &args) {
            return atan2(-arg_number(args, 1), arg_number(args, 0));
        });

        simple("getdir", [](const values &args) {
            const auto delta_x = arg_number(args, 0);
            const auto delta_y = arg_number(args, 1);

            if (fabs(delta_x) > fabs(delta_y)) {
                return delta_x < 0.0 ? 1.0 : 3.0;
            }

            return delta_y < 0.0 ? 0.0 : 2.0;
        });

        env.register_function("format", [](environment &, const values &args) -> expected_value {
            if (args.empty()) {
                return value{string{}};
            }

            return value{format_string(to_string(args[0]), args, 1)};
        });

        env.register_function("lowercase", [](environment &, const values &args) -> expected_value {
            auto text = to_string(arg(args, 0));
            ranges::transform(text, text.begin(), [](const unsigned char ch) {
                return static_cast<char>(tolower(ch));
            });

            return value{text};
        });

        env.register_function("uppercase", [](environment &, const values &args) -> expected_value {
            auto text = to_string(arg(args, 0));
            ranges::transform(text, text.begin(), [](const unsigned char ch) {
                return static_cast<char>(toupper(ch));
            });

            return value{text};
        });

        env.register_function("pi", [](environment &, const values &) -> expected_value {
            return value{pi};
        });

        env.put("join",
            value{make_shared<native_context_function>(
                [](context &ctx, const values &args) -> expected_value {
                    auto *target = ctx.get_script();
                    if (target == nullptr || args.empty()) {
                        return value{0.0};
                    }

                    const auto class_name = to_string(args.front());

                    auto joined = ctx.get_environment().find_class(class_name);
                    if (!joined) {
                        return unexpected(error{
                            .source = error_source::interpreter,
                            .kind = error_kind::runtime_error,
                            .message = std::format("unknown class '{}'", class_name),
                        });
                    }

                    return value{target->join(joined) ? 1.0 : 0.0};
                })});

        env.put("makevar",
            value{make_shared<native_context_function>(
                [](context &ctx, const values &args) -> expected_value {
                    if (args.empty()) {
                        return value{};
                    }

                    return resolve_path(ctx, to_string(args.front()));
                })});

        env.put("unset",
            value{make_shared<native_context_function>(
                [](context &ctx, const values &args) -> expected_value {
                    if (args.empty()) {
                        return value{0.0};
                    }

                    const auto path = to_string(args.front());
                    const auto dot = path.rfind('.');

                    if (dot == string::npos) {
                        const auto scope = ctx.find_scope(path);

                        return value{scope && scope->erase(path) ? 1.0 : 0.0};
                    }

                    const auto owner = resolve_path(ctx, string_view(path).substr(0, dot));
                    const auto *dictionary = get_if<dictionary_ptr>(&owner);

                    if (dictionary == nullptr || !*dictionary) {
                        return value{0.0};
                    }

                    return value{(*dictionary)->erase(string_view(path).substr(dot + 1)) ? 1.0 : 0.0};
                })});
    }
}
