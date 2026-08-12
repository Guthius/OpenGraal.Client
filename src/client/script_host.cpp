#include "script_host.hpp"

#include "sound_manager.hpp"

#include "chat_bar.hpp"
#include "constants.hpp"
#include "dev_input.hpp"
#include "file_manager.hpp"
#include "font_manager.hpp"
#include "game.hpp"
#include "gui/gui_bitmap_button_ctrl.hpp"
#include "gui/gui_button_base_ctrl.hpp"
#include "gui/gui_button_ctrl.hpp"
#include "gui/gui_check_box_ctrl.hpp"
#include "gui/gui_context_menu_ctrl.hpp"
#include "gui/gui_control_profile.hpp"
#include "gui/gui_drawing_panel.hpp"
#include "gui/gui_ml_text_ctrl.hpp"
#include "gui/gui_ml_text_edit_ctrl.hpp"
#include "gui/gui_popup_menu_ctrl.hpp"
#include "gui/gui_progress_ctrl.hpp"
#include "gui/gui_radio_ctrl.hpp"
#include "gui/gui_scroll_ctrl.hpp"
#include "gui/gui_show_img_ctrl.hpp"
#include "gui/gui_slider_ctrl.hpp"
#include "gui/gui_tab_ctrl.hpp"
#include "gui/gui_text_ctrl.hpp"
#include "gui/gui_text_edit_ctrl.hpp"
#include "gui/gui_text_list_ctrl.hpp"
#include "gui/gui_tree_view_ctrl.hpp"
#include "gui/gui_window_ctrl.hpp"
#include "key_codes.hpp"
#include "network.hpp"
#include "player.hpp"
#include "remote_player.hpp"
#include "texture_manager.hpp"
#include "tileset_manager.hpp"

#include <fstream>
#include <gs2/builtins.hpp>
#include <sstream>
#include <utility>

using namespace og::gs2;

namespace {
    script_host instance;

    auto number_at(const values &args, const size_t index) -> double {
        return index < args.size() ? to_number(args[index]) : 0.0;
    }

    auto text_at(const values &args, const size_t index) -> std::string {
        return index < args.size() ? to_string(args[index]) : std::string{};
    }

    struct host_method : callable {
        using body = std::function<expected_value(const values &args)>;

        body invoke_body;

        explicit host_method(body invoke_body) : invoke_body(std::move(invoke_body)) {
        }

        auto invoke(const values &args) -> expected_value override {
            return invoke_body(args);
        }
    };

    auto is_name(const std::string_view name, const std::string_view expected) -> bool {
        return folded_equal{}(name, expected);
    }

    constexpr float base_text_size = 12.0f;

    auto parse_color(const std::string &text) -> Color {
        std::array channels = {255.0, 255.0, 255.0, 255.0};

        std::istringstream is(text);
        for (auto &channel : channels) {
            double parsed = 0.0;
            if (!(is >> parsed)) {
                break;
            }

            channel = std::clamp(parsed, 0.0, 255.0);
        }

        return Color{
            static_cast<unsigned char>(channels[0]),
            static_cast<unsigned char>(channels[1]),
            static_cast<unsigned char>(channels[2]),
            static_cast<unsigned char>(channels[3])};
    }

    auto to_color_text(const Color &color) -> std::string {
        return std::format("{} {} {} {}", color.r, color.g, color.b, color.a);
    }

    auto parse_pair(const std::string &text) -> Vector2 {
        std::istringstream is(text);
        Vector2 pair{};

        is >> pair.x >> pair.y;

        return pair;
    }

    auto to_pair_text(const Vector2 &pair) -> std::string {
        return std::format("{} {}", pair.x, pair.y);
    }

    class show_img_actor final : public basic_dictionary {
      public:
        explicit show_img_actor(std::shared_ptr<gui_show_img_ctrl> control) : control_(std::move(control)) {
        }

        auto contains(const std::string_view name) -> bool override {
            return field_of(name) != nullptr || is_name(name, "attr") || is_name(name, "dir") ||
                   basic_dictionary::contains(name);
        }

        auto get(const std::string_view name) -> value override {
            if (is_name(name, "attr")) {
                return value{attr_};
            }

            if (is_name(name, "dir")) {
                return value{static_cast<double>(control_->get_direction())};
            }

            if (const auto *field = field_of(name)) {
                return value{*field};
            }

            return basic_dictionary::get(name);
        }

        void put(const std::string_view name, value val) override {
            if (is_name(name, "dir")) {
                control_->set_direction(static_cast<direction>(static_cast<int>(to_number(val)) & 3));

                return;
            }

            if (auto *field = field_of(name)) {
                *field = to_string(val);

                return;
            }

            basic_dictionary::put(name, std::move(val));
        }

        void sync() const {
            auto &state = control_->get_state();

            for (size_t i = 0; i < attr_->elements.size() && i < state.attr.size(); ++i) {
                state.attr[i] = to_string(attr_->elements[i]);
            }
        }

      private:
        auto field_of(const std::string_view name) const -> std::string * {
            auto &state = control_->get_state();

            if (is_name(name, "headimg")) {
                return &state.head;
            }

            if (is_name(name, "bodyimg")) {
                return &state.body;
            }

            return nullptr;
        }

        std::shared_ptr<gui_show_img_ctrl> control_;
        array_ptr attr_ = std::make_shared<array>(values(gani_attr_count));
    };

    class selected_row final : public basic_dictionary {
      public:
        explicit selected_row(std::shared_ptr<gui_text_list_ctrl> control) : control_(std::move(control)) {
        }

        auto contains(const std::string_view name) -> bool override {
            return is_name(name, "gettext") || is_name(name, "id") || is_name(name, "text") ||
                   basic_dictionary::contains(name);
        }

        auto get(const std::string_view name) -> value override {
            if (is_name(name, "gettext")) {
                auto control = control_;

                return value{std::make_shared<host_method>([control](const values &) -> expected_value {
                    return value{control->get_selected_text()};
                })};
            }

            if (is_name(name, "text")) {
                return value{control_->get_selected_text()};
            }

            if (is_name(name, "id")) {
                return value{static_cast<double>(control_->get_selected_row())};
            }

            return basic_dictionary::get(name);
        }

      private:
        std::shared_ptr<gui_text_list_ctrl> control_;
    };

    class show_img_holder final : public basic_dictionary {
      public:
        explicit show_img_holder(std::shared_ptr<gui_show_img_ctrl> control) : control_(std::move(control)) {
        }

        auto contains(const std::string_view name) -> bool override {
            return is_name(name, "image") || is_name(name, "ani") || basic_dictionary::contains(name);
        }

        auto get(const std::string_view name) -> value override {
            if (is_name(name, "image")) {
                return value{control_->get_image()};
            }

            if (is_name(name, "ani")) {
                return value{control_->get_ani()};
            }

            return basic_dictionary::get(name);
        }

        void put(const std::string_view name, value val) override {
            if (is_name(name, "image")) {
                control_->set_image(to_string(val));

                return;
            }

            if (is_name(name, "ani")) {
                control_->set_ani(to_string(val));

                return;
            }

            basic_dictionary::put(name, std::move(val));
        }

      private:
        std::shared_ptr<gui_show_img_ctrl> control_;
    };

    auto to_color(const show_entry &entry) -> Color {
        const auto channel = [](const float value) {
            return static_cast<unsigned char>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
        };

        return Color{channel(entry.red), channel(entry.green), channel(entry.blue), channel(entry.alpha)};
    }
}

void script_host::client_flag_store::put(const std::string_view name, value val) {
    basic_dictionary::put(name, std::move(val));
    changed_.emplace_back(name);
}

void script_host::client_flag_store::accept(const std::string_view name, value val) {
    basic_dictionary::put(name, std::move(val));
}

auto script_host::client_flag_store::take_changed() -> std::vector<std::string> {
    return std::exchange(changed_, {});
}

auto script_object::contains(const std::string_view name) -> bool {
    return is_name(name, "catchevent") || basic_dictionary::contains(name) || has_function(name);
}

auto script_object::has_function(const std::string_view name) const -> bool {
    return owner_->script && owner_->script->has_function(name);
}

auto script_object::get(const std::string_view name) -> value {
    if (is_name(name, "catchevent")) {
        return value{std::make_shared<host_method>([host = host_, owner = owner_](const values &args) -> expected_value {
            host->catch_event(owner, text_at(args, 0), text_at(args, 1), text_at(args, 2));

            return value{};
        })};
    }

    if (!basic_dictionary::contains(name) && has_function(name)) {
        return value{std::make_shared<host_method>(
            [host = host_, owner = owner_, function = std::string(name)](const values &args) -> expected_value {
                return host->call_weapon(*owner, function, args);
            })};
    }

    return basic_dictionary::get(name);
}

auto get_script_host() -> script_host & {
    return instance;
}

script_host::script_host() {
    env_.set_file_resolver([](const std::string_view name) -> std::optional<std::string> {
        const auto filename = std::filesystem::path(name).filename().string();
        if (filename.empty()) {
            return std::nullopt;
        }

        auto path = std::filesystem::path("scriptdata") / filename;
        if (!exists(path)) {
            if (!has_file(filename)) {
                return std::nullopt;
            }

            path = find_file(filename);
        }

        std::ifstream is(path, std::ios::in | std::ios::binary);
        if (!is.is_open()) {
            return std::nullopt;
        }

        return std::string(std::istreambuf_iterator(is), std::istreambuf_iterator<char>());
    });

    env_.set_file_writer([](const std::string_view name, const std::string_view contents) -> bool {
        const auto filename = std::filesystem::path(name).filename();
        if (filename.empty()) {
            return false;
        }

        std::error_code failed;
        std::filesystem::create_directories("scriptdata", failed);

        std::ofstream os(std::filesystem::path("scriptdata") / filename, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!os.is_open()) {
            return false;
        }

        os << contents;

        return os.good();
    });

    env_.set_object_sink([this](const dictionary_ptr &object, const dictionary_ptr &enclosing) {
        auto child = find_instance(object);
        auto parent = find_instance(enclosing);

        if (child && parent) {
            parent->add_child(child);
        }

        if (auto preview = std::dynamic_pointer_cast<gui_show_img_ctrl>(child)) {
            auto actor = std::make_shared<show_img_actor>(preview);
            auto holder = std::make_shared<show_img_holder>(std::move(preview));

            holder->put("actor", value{actor});
            object->put("showimg", value{holder});

            show_img_actors_.emplace_back([actor] { actor->sync(); });
        }

        if (auto list = std::dynamic_pointer_cast<gui_text_list_ctrl>(child)) {
            object->put("selected", value{std::make_shared<selected_row>(std::move(list))});
        }
    });

    bind_show_type();
    bind_player_type();
    bind();
    bind_preferences();
    bind_profile_types();
    bind_gui_types();
}

void script_host::bind_preferences() {
    auto prefs = std::make_shared<basic_dictionary>();
    auto graal = std::make_shared<basic_dictionary>();

    graal->put("defaultfontsize", value{static_cast<double>(base_text_size * 2.0f)});
    graal->put("defaultfontname", value{std::string("LiberationSans")});

    prefs->put("graal", value{graal});

    env_.put("$pref", value{prefs});
}

namespace {
    auto make_profile(const std::string &texture, const bool bold = false) -> std::shared_ptr<gui_control_profile> {
        auto profile = std::make_shared<gui_control_profile>();

        profile->set_texture(texture);
        profile->set_font(bold ? "LiberationSans-Bold.ttf" : "LiberationSans-Regular.ttf");

        return profile;
    }

    template <typename TControl>
    auto adopt(const values &args, const std::string &texture) -> std::shared_ptr<TControl> {
        auto control = std::make_shared<TControl>();

        control->set_profile(make_profile(texture));
        control->set_position({0, 0});
        control->set_size({120, 24});

        const auto name = args.empty() ? std::string{} : to_string(args[0]);

        control->set_name(name);
        get_script_host().register_control(name, control);
        get_script_host().register_instance(control);
        get_script_host().attach_control(control);

        return control;
    }

    struct particle_emitter {};

    auto to_offsets(const std::string &text) -> std::vector<float> {
        auto spaced = text;
        std::ranges::replace(spaced, ',', ' ');

        std::istringstream is(spaced);
        std::vector<float> offsets;

        for (float offset = 0; is >> offset;) {
            offsets.push_back(offset);
        }

        return offsets;
    }

    auto to_lines(const values &args) -> std::vector<std::string> {
        std::vector<std::string> lines;

        if (!args.empty()) {
            if (const auto *source = std::get_if<array_ptr>(&args[0]); source != nullptr && *source) {
                for (const auto &line : (*source)->elements) {
                    lines.push_back(to_string(line));
                }
            }
        }

        return lines;
    }

    template <typename TBuilder>
    auto with_common(TBuilder &builder) -> TBuilder & {
        builder.function("destroy", [](gui_control *self, const values &) -> expected_value {
            get_script_host().destroy_control(self->get_name());

            return value{};
        });

        builder.function("bringtofront", [](const gui_control *self, const values &) -> expected_value {
            self->bring_to_front();

            return value{};
        });
        builder.function("pushtoback", [](const gui_control *self, const values &) -> expected_value {
            self->push_to_back();

            return value{};
        });

        builder.function("makefirstresponder", [](gui_control *self, const values &args) -> expected_value {
            if (args.empty() || to_bool(args[0])) {
                self->focus();
            } else {
                self->blur();
            }

            return value{};
        });

        return builder
            .property("name", [](const gui_control *self) { return self->get_name(); })
            .property(
                "profile",
                [](const gui_control *self) { return self->get_profile() ? self->get_profile()->get_name() : std::string{}; },
                [](gui_control *self, const std::string &value) {
                    if (auto profile = get_script_host().find_profile(value)) {
                        self->set_profile(profile);
                    }
                })
            .property(
                "x",
                [](const gui_control *self) { return self->get_position().x; },
                [](gui_control *self, const float value) { self->set_position({value, self->get_position().y}); })
            .property(
                "y",
                [](const gui_control *self) { return self->get_position().y; },
                [](gui_control *self, const float value) { self->set_position({self->get_position().x, value}); })
            .property(
                "width",
                [](const gui_control *self) { return self->get_size().x; },
                [](gui_control *self, const float value) { self->set_size({value, self->get_size().y}); })
            .property(
                "height",
                [](const gui_control *self) { return self->get_size().y; },
                [](gui_control *self, const float value) { self->set_size({self->get_size().x, value}); })
            .property(
                "visible",
                [](const gui_control *self) { return self->is_visible(); },
                [](gui_control *self, const bool value) { self->set_visible(value); })
            .property(
                "position",
                [](const gui_control *self) { return to_pair_text(self->get_position()); },
                [](gui_control *self, const std::string &value) { self->set_position(parse_pair(value)); })
            .property(
                "extent",
                [](const gui_control *self) { return to_pair_text(self->get_size()); },
                [](gui_control *self, const std::string &value) { self->set_size(parse_pair(value)); });
    }

    template <typename TBuilder>
    auto with_checked(TBuilder &builder) -> TBuilder & {
        return builder.property(
            "checked",
            [](const gui_button_base_ctrl *self) { return self->is_checked(); },
            [](gui_button_base_ctrl *self, const bool value) { self->set_checked(value); });
    }

    template <typename TBuilder>
    auto with_text(TBuilder &builder) -> TBuilder & {
        builder.function("settext", [](gui_text_ctrl *self, const values &args) -> expected_value {
            self->set_text(text_at(args, 0));

            return value{};
        });
        builder.function("gettext", [](const gui_text_ctrl *self, const values &) -> expected_value {
            return value{self->get_text()};
        });

        return builder.property(
            "text",
            [](const gui_text_ctrl *self) { return self->get_text(); },
            [](gui_text_ctrl *self, const std::string &value) { self->set_text(value); });
    }
}

void script_host::bind_emitter_type() {
    auto builder = prototype_builder<particle_emitter>("TParticleEmitter");

    for (const auto *name : {"addlocalmodifier", "addglobalmodifier", "addmodifier", "removemodifiers"}) {
        builder.function(name, [](particle_emitter *, const values &) -> expected_value { return value{}; });
    }

    emitter_proto_ = builder.build();
    env_.register_type(emitter_proto_);
}

void script_host::bind_show_type() {
    bind_emitter_type();

    auto builder = prototype_builder<show_entry>("TShowImg");

    const auto number = [&builder](const std::string_view name, float show_entry::*field) -> auto & {
        return builder.property(
            name,
            [field](const show_entry *self) { return self->*field; },
            [field](show_entry *self, const float value) { self->*field = value; });
    };

    const auto text = [&builder](const std::string_view name, std::string show_entry::*field) -> auto & {
        return builder.property(
            name,
            [field](const show_entry *self) { return self->*field; },
            [field](show_entry *self, const std::string &value) { self->*field = value; });
    };

    text("text", &show_entry::text);
    text("image", &show_entry::image);
    text("font", &show_entry::font);
    text("style", &show_entry::style);

    number("zoom", &show_entry::zoom);
    number("red", &show_entry::red);
    number("green", &show_entry::green);
    number("blue", &show_entry::blue);
    number("alpha", &show_entry::alpha);

    builder.property(
        "x",
        [](const show_entry *self) { return self->position.x; },
        [](show_entry *self, const float value) { self->position.x = value; });
    builder.property(
        "y",
        [](const show_entry *self) { return self->position.y; },
        [](show_entry *self, const float value) { self->position.y = value; });
    builder.property(
        "layer",
        [](const show_entry *self) { return self->layer; },
        [](show_entry *self, const int value) { self->layer = value; });
    builder.property(
        "visible",
        [](const show_entry *self) { return self->visible; },
        [](show_entry *self, const bool value) { self->visible = value; });
    builder.property("emitter", [](const show_entry *self) { return value{self->emitter}; });

    show_proto_ = builder.build();
}

void script_host::bind_player_type() {
    auto builder = prototype_builder<player>("TPlayer");

    builder.property(
        "x",
        [](const player *self) { return self->get_position().x / tile_size; },
        [](player *self, const float value) { self->set_position({value * tile_size, self->get_position().y}); });
    builder.property(
        "y",
        [](const player *self) { return self->get_position().y / tile_size; },
        [](player *self, const float value) { self->set_position({self->get_position().x, value * tile_size}); });
    builder.property(
        "dir",
        [](const player *self) { return static_cast<int>(self->get_direction()); },
        [](player *self, const int value) { self->set_direction(static_cast<direction>(value & 3)); });
    builder.property(
        "ani",
        [](const player *self) { return self->get_animation(); },
        [](player *self, const std::string &value) { self->set_animation(value); });

    builder.property("headimg", [](const player *self) { return self->get_animation_state().head; });
    builder.property("bodyimg", [](const player *self) { return self->get_animation_state().body; });
    builder.property("attr", [](const player *self) {
        const auto &state = self->get_animation_state();
        auto attr = std::make_shared<array>();

        for (const auto &entry : state.attr) {
            attr->elements.emplace_back(entry);
        }

        return value{attr};
    });

    builder.property("freezetime", [](const player *) { return -1.0; });
    builder.property("z", [](const player *) { return 0.0; });

    player_proto_ = builder.build();
}

void script_host::bind_profile_types() {
    static constexpr std::array names = {
        "GuiControlProfile", "GuiTextProfile", "GuiScrollProfile", "GuiProgressProfile", "GuiMLTextEditProfile"};

    for (const auto *type_name : names) {
        auto builder = prototype_builder<gui_control_profile>(type_name);

        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            auto profile = make_profile("guiblue_window.png");
            auto name = args.empty() ? std::string{} : to_string(args[0]);

            profile->set_name(name);
            get_script_host().register_profile(name, profile);

            return profile;
        });

        const auto color = [&builder](const std::string_view name, Color gui_control_profile::*field) -> auto & {
            return builder.property(
                name,
                [field](const gui_control_profile *self) { return to_color_text(self->*field); },
                [field](gui_control_profile *self, const std::string &value) { self->*field = parse_color(value); });
        };

        color("fontcolor", &gui_control_profile::font_color);
        color("fontcolorhl", &gui_control_profile::font_color_hl);
        color("fontcolorna", &gui_control_profile::font_color_na);
        color("fontcolorsel", &gui_control_profile::font_color_sel);
        color("fontcolorlink", &gui_control_profile::font_color_link);
        color("fontcolorlinkhl", &gui_control_profile::font_color_link_hl);
        color("fillcolor", &gui_control_profile::fill_color);
        color("fillcolorhl", &gui_control_profile::fill_color_hl);
        color("fillcolorna", &gui_control_profile::fill_color_na);
        color("bordercolor", &gui_control_profile::border_color);
        color("bordercolorhl", &gui_control_profile::border_color_hl);
        color("bordercolorna", &gui_control_profile::border_color_na);
        color("cursorcolor", &gui_control_profile::caret_color);

        builder.property(
            "name",
            [](const gui_control_profile *self) { return self->get_name(); });
        builder.property(
            "fontsize",
            [](const gui_control_profile *self) { return self->get_font_size(); },
            [](gui_control_profile *self, const float value) { self->set_font_size(value); });
        builder.property(
            "bitmap",
            [](const gui_control_profile *self) { return self->get_texture_filename(); },
            [](gui_control_profile *self, const std::string &value) { self->set_texture(value); });
        builder.property(
            "border",
            [](const gui_control_profile *self) { return static_cast<float>(self->border_style); },
            [](gui_control_profile *self, const float value) {
                self->border_style = value >= 0.0f && value <= 4.0f
                                         ? static_cast<gui_border_style>(static_cast<uint8_t>(value))
                                         : gui_border_style::none;
            });

        env_.register_type(builder.build());
    }
}

void script_host::bind_gui_types() {
    {
        auto builder = prototype_builder<gui_control>("GuiControl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            return adopt<gui_control>(args, "guiblue_window.png");
        });
        with_common(builder);
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_window_ctrl>("GuiWindowCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            auto control = adopt<gui_window_ctrl>(args, "guiblue_window.png");
            control->set_size({240, 140});

            return control;
        });
        with_common(builder);
        builder.property(
            "text",
            [](const gui_window_ctrl *self) { return self->get_text(); },
            [](gui_window_ctrl *self, const std::string &value) { self->set_text(value); });
        builder.property(
            "canmove",
            [](const gui_window_ctrl *self) { return self->is_draggable(); },
            [](gui_window_ctrl *self, const bool value) { self->set_draggable(value); });
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_text_ctrl>("GuiTextCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            auto control = adopt<gui_text_ctrl>(args, "guiblue_radio.png");
            control->set_fit_to_text(true);

            return control;
        });
        with_common(builder);
        with_text(builder);
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_drawing_panel>("GuiDrawingPanel");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            return adopt<gui_drawing_panel>(args, "guiblue_window.png");
        });
        with_common(builder);
        builder.function("clearall", [](gui_drawing_panel *self, const values &) -> expected_value {
            self->clear_all();

            return value{};
        });
        builder.function("drawimage", [](gui_drawing_panel *self, const values &args) -> expected_value {
            self->draw_image(text_at(args, 2), {static_cast<float>(number_at(args, 0)), static_cast<float>(number_at(args, 1))});

            return value{};
        });
        builder.function("drawimagestretched", [](gui_drawing_panel *self, const values &args) -> expected_value {
            const auto at = [&args](const size_t index) { return static_cast<float>(number_at(args, index)); };

            self->draw_image_stretched(
                text_at(args, 4),
                {at(0), at(1), at(2), at(3)},
                {at(5), at(6), at(7), at(8)});

            return value{};
        });
        builder.function("drawimagerectangle", [](gui_drawing_panel *self, const values &args) -> expected_value {
            const auto at = [&args](const size_t index) { return static_cast<float>(number_at(args, index)); };

            self->draw_image_rectangle(text_at(args, 2), {at(0), at(1)}, {at(3), at(4), at(5), at(6)});

            return value{};
        });
        builder.function("lock", [](gui_drawing_panel *, const values &) -> expected_value { return value{}; });
        builder.function("unlock", [](gui_drawing_panel *, const values &) -> expected_value { return value{}; });
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_context_menu_ctrl>("GuiContextMenuCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            return adopt<gui_context_menu_ctrl>(args, "guiblue_window.png");
        });
        with_common(builder);
        builder.function("addrow", [](gui_context_menu_ctrl *self, const values &args) -> expected_value {
            self->add_row(static_cast<int>(number_at(args, 0)), text_at(args, 1));

            return value{};
        });
        builder.function("clearrows", [](gui_context_menu_ctrl *self, const values &) -> expected_value {
            self->clear_rows();

            return value{};
        });
        builder.function("getselectedrow", [](const gui_context_menu_ctrl *self, const values &) -> expected_value {
            return value{static_cast<double>(self->get_selected_row())};
        });
        builder.function("open", [](gui_context_menu_ctrl *self, const values &args) -> expected_value {
            self->open({static_cast<float>(number_at(args, 0)), static_cast<float>(number_at(args, 1))});

            return value{};
        });
        builder.function("show", [](gui_context_menu_ctrl *self, const values &args) -> expected_value {
            self->open({static_cast<float>(number_at(args, 0)), static_cast<float>(number_at(args, 1))});

            return value{};
        });
        builder.function("close", [](gui_context_menu_ctrl *self, const values &) -> expected_value {
            self->close();

            return value{};
        });
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_bitmap_button_ctrl>("GuiBitmapButtonCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            return adopt<gui_bitmap_button_ctrl>(args, "guiblue_button.png");
        });
        with_common(builder);
        with_text(builder);
        builder.property(
            "normalbitmap",
            [](const gui_bitmap_button_ctrl *self) { return self->get_normal_bitmap(); },
            [](gui_bitmap_button_ctrl *self, const std::string &value) { self->set_normal_bitmap(value); });
        builder.property(
            "mouseoverbitmap",
            [](const gui_bitmap_button_ctrl *self) { return self->get_mouse_over_bitmap(); },
            [](gui_bitmap_button_ctrl *self, const std::string &value) { self->set_mouse_over_bitmap(value); });
        builder.property(
            "pressedbitmap",
            [](const gui_bitmap_button_ctrl *self) { return self->get_pressed_bitmap(); },
            [](gui_bitmap_button_ctrl *self, const std::string &value) { self->set_pressed_bitmap(value); });
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_scroll_ctrl>("GuiScrollCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            return adopt<gui_scroll_ctrl>(args, "guiblue_scroll.png");
        });
        with_common(builder);
        builder.property(
            "hscrollbar",
            [](const gui_scroll_ctrl *self) { return self->get_h_scroll_bar(); },
            [](gui_scroll_ctrl *self, const std::string &value) { self->set_h_scroll_bar(value); });
        builder.property(
            "vscrollbar",
            [](const gui_scroll_ctrl *self) { return self->get_v_scroll_bar(); },
            [](gui_scroll_ctrl *self, const std::string &value) { self->set_v_scroll_bar(value); });
        builder.property(
            "tile",
            [](const gui_scroll_ctrl *self) { return self->is_tiled(); },
            [](gui_scroll_ctrl *self, const bool value) { self->set_tiled(value); });
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_progress_ctrl>("GuiProgressCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            return adopt<gui_progress_ctrl>(args, "guiblue_window.png");
        });
        with_common(builder);
        builder.property(
            "progress",
            [](const gui_progress_ctrl *self) { return self->get_progress(); },
            [](gui_progress_ctrl *self, const float value) { self->set_progress(value); });
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_show_img_ctrl>("GuiShowImgCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            auto control = adopt<gui_show_img_ctrl>(args, "guiblue_window.png");

            control->set_profile(nullptr);

            return control;
        });

        with_common(builder);

        builder.property(
            "image",
            [](const gui_show_img_ctrl *self) { return self->get_image(); },
            [](gui_show_img_ctrl *self, const std::string &value) { self->set_image(value); });
        builder.property(
            "ani",
            [](const gui_show_img_ctrl *self) { return self->get_ani(); },
            [](gui_show_img_ctrl *self, const std::string &value) { self->set_ani(value); });
        builder.property(
            "offsetx",
            [](const gui_show_img_ctrl *self) { return self->get_offset().x; },
            [](gui_show_img_ctrl *self, const float value) { self->set_offset({value, self->get_offset().y}); });
        builder.property(
            "offsety",
            [](const gui_show_img_ctrl *self) { return self->get_offset().y; },
            [](gui_show_img_ctrl *self, const float value) { self->set_offset({self->get_offset().x, value}); });
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_slider_ctrl>("GuiSliderCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            return adopt<gui_slider_ctrl>(args, "guiblue_slider.png");
        });
        with_common(builder);
        builder.property(
            "value",
            [](const gui_slider_ctrl *self) { return self->get_value(); },
            [](gui_slider_ctrl *self, const float value) { self->set_value(value); });

        builder.property(
            "range",
            [](const gui_slider_ctrl *self) { return to_pair_text({self->get_minimum(), self->get_maximum()}); },
            [](gui_slider_ctrl *self, const std::string &value) {
                const auto [minimum, maximum] = parse_pair(value);

                self->set_range(minimum, maximum);
            });
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_tab_ctrl>("GuiTabCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            return adopt<gui_tab_ctrl>(args, "guiblue_tab.png");
        });
        with_common(builder);
        builder.function("clearrows", [](gui_tab_ctrl *self, const values &) -> expected_value {
            self->clear_rows();

            return value{};
        });
        builder.function("addrow", [](gui_tab_ctrl *self, const values &args) -> expected_value {
            self->add_row(static_cast<int>(number_at(args, 0)), text_at(args, 1));

            return value{};
        });
        builder.function("setselectedrow", [](gui_tab_ctrl *self, const values &args) -> expected_value {
            self->set_selected_row(static_cast<int>(number_at(args, 0)));

            return value{};
        });
        builder.function("getselectedrow", [](const gui_tab_ctrl *self, const values &) -> expected_value {
            return value{static_cast<double>(self->get_selected_row())};
        });
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_text_list_ctrl>("GuiTextListCtrl");

        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            return adopt<gui_text_list_ctrl>(args, "guiblue_scroll.png");
        });

        with_common(builder);

        builder.property(
            "textprofile",
            [](const gui_text_list_ctrl *) { return std::string{}; },
            [](gui_text_list_ctrl *self, const std::string &value) {
                if (auto profile = get_script_host().find_profile(value)) {
                    self->set_text_profile(profile);
                }
            });

        builder.function("clearrows", [](gui_text_list_ctrl *self, const values &) -> expected_value {
            self->clear_rows();

            return value{};
        });

        builder.function("addrow", [](gui_text_list_ctrl *self, const values &args) -> expected_value {
            self->add_row(static_cast<int>(number_at(args, 0)), text_at(args, 1));

            return value{};
        });

        builder.function("removerow", [](gui_text_list_ctrl *self, const values &args) -> expected_value {
            self->remove_row(static_cast<int>(number_at(args, 0)));

            return value{};
        });

        builder.function("setselectedrow", [](gui_text_list_ctrl *self, const values &args) -> expected_value {
            self->set_selected_row(static_cast<int>(number_at(args, 0)));

            return value{};
        });

        builder.function("getselectedrow", [](const gui_text_list_ctrl *self, const values &) -> expected_value {
            return value{static_cast<double>(self->get_selected_row())};
        });

        builder.function("getrowcount", [](const gui_text_list_ctrl *self, const values &) -> expected_value {
            return value{static_cast<double>(self->get_rows().size())};
        });

        builder.function("gettext", [](const gui_text_list_ctrl *self, const values &) -> expected_value {
            return value{self->get_selected_text()};
        });

        builder.function("getselectedtext", [](const gui_text_list_ctrl *self, const values &) -> expected_value {
            return value{self->get_selected_text()};
        });

        builder.function("rowcount", [](const gui_text_list_ctrl *self, const values &) -> expected_value {
            return value{static_cast<double>(self->get_rows().size())};
        });

        builder.function("sort", [](gui_text_list_ctrl *self, const values &) -> expected_value {
            self->sort();

            return value{};
        });

        builder.function("seticonsize", [](gui_text_list_ctrl *, const values &) -> expected_value { return value{}; });
        builder.property(
            "columns",
            [](const gui_text_list_ctrl *) { return std::string{}; },
            [](gui_text_list_ctrl *self, const std::string &value) { self->set_columns(to_offsets(value)); });
        builder.property("rows", [](const gui_text_list_ctrl *self) {
            auto rows = std::make_shared<array>();

            for (const auto &row : self->get_rows()) {
                auto entry = std::make_shared<basic_dictionary>();

                entry->put("id", value{static_cast<double>(row.id)});
                entry->put("text", value{row.text});

                rows->elements.emplace_back(entry);
            }

            return value{rows};
        });
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_ml_text_ctrl>("GuiMLTextCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            return adopt<gui_ml_text_ctrl>(args, "guiblue_radio.png");
        });
        with_common(builder);
        builder.property(
            "text",
            [](const gui_ml_text_ctrl *self) { return self->get_text(); },
            [](gui_ml_text_ctrl *self, const std::string &value) { self->set_text(value); });
        builder.property(
            "wordwrap",
            [](const gui_ml_text_ctrl *self) { return self->is_word_wrap(); },
            [](gui_ml_text_ctrl *self, const bool value) { self->set_word_wrap(value); });
        builder.function("settext", [](gui_ml_text_ctrl *self, const values &args) -> expected_value {
            self->set_text(text_at(args, 0));

            return value{};
        });
        builder.function("gettext", [](const gui_ml_text_ctrl *self, const values &) -> expected_value {
            return value{self->get_text()};
        });
        builder.function("addtext", [](gui_ml_text_ctrl *self, const values &args) -> expected_value {
            self->add_text(text_at(args, 0));

            return value{};
        });
        builder.function("getlinecount", [](const gui_ml_text_ctrl *self, const values &) -> expected_value {
            return value{static_cast<double>(self->get_line_count())};
        });
        builder.function("getlines", [](const gui_ml_text_ctrl *self, const values &) -> expected_value {
            auto lines = std::make_shared<array>();
            for (const auto &line : self->get_lines()) {
                lines->elements.emplace_back(line);
            }

            return value{lines};
        });
        builder.function("setlines", [](gui_ml_text_ctrl *self, const values &args) -> expected_value {
            self->set_lines(to_lines(args));

            return value{};
        });
        builder.function("scrolltobottom", [](gui_ml_text_ctrl *self, const values &) -> expected_value {
            self->scroll_to_bottom();

            return value{};
        });
        builder.function("scrolltotop", [](gui_ml_text_ctrl *self, const values &) -> expected_value {
            self->scroll_to_top();

            return value{};
        });
        builder.function("reflow", [](gui_ml_text_ctrl *, const values &) -> expected_value { return value{}; });
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_button_ctrl>("GuiButtonCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            return adopt<gui_button_ctrl>(args, "guiblue_button.png");
        });
        with_common(builder);
        with_text(builder);
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_text_edit_ctrl>("GuiTextEditCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            auto control = adopt<gui_text_edit_ctrl>(args, "guiblue_textedit.png");
            control->get_profile()->fill_color = {41, 82, 156, 255};
            control->get_profile()->fill_color_hl = {41, 82, 156, 255};

            return control;
        });
        with_common(builder);
        with_text(builder);
        builder.property(
            "password",
            [](const gui_text_edit_ctrl *self) { return self->is_password(); },
            [](gui_text_edit_ctrl *self, const bool value) { self->set_password(value); });
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_check_box_ctrl>("GuiCheckBoxCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            return adopt<gui_check_box_ctrl>(args, "guiblue_check.png");
        });
        with_common(builder);
        with_text(builder);
        with_checked(builder);
        env_.register_type(builder.build());
    }

    {
        auto builder = prototype_builder<gui_radio_ctrl>("GuiRadioCtrl");
        builder.constructor([](const values &args) -> std::shared_ptr<void> {
            return adopt<gui_radio_ctrl>(args, "guiblue_radio.png");
        });
        with_common(builder);
        with_text(builder);
        with_checked(builder);
        env_.register_type(builder.build());
    }

    bind_ml_text_edit_type();
    bind_popup_menu_type();
    bind_tree_view_types();
}

void script_host::bind_ml_text_edit_type() {
    auto builder = prototype_builder<gui_ml_text_edit_ctrl>("GuiMLTextEditCtrl");

    builder.constructor([](const values &args) -> std::shared_ptr<void> {
        return adopt<gui_ml_text_edit_ctrl>(args, "guiblue_scroll.png");
    });

    with_common(builder);

    builder.property(
        "text",
        [](const gui_ml_text_edit_ctrl *self) { return self->get_text(); },
        [](gui_ml_text_edit_ctrl *self, const std::string &value) { self->set_text(value); });

    builder.property(
        "wordwrap",
        [](const gui_ml_text_edit_ctrl *self) { return self->is_word_wrap(); },
        [](gui_ml_text_edit_ctrl *self, const bool value) { self->set_word_wrap(value); });

    builder.property(
        "parsetags",
        [](const gui_ml_text_edit_ctrl *self) { return self->is_parse_tags(); },
        [](gui_ml_text_edit_ctrl *self, const bool value) { self->set_parse_tags(value); });

    builder.property(
        "autoindenting",
        [](const gui_ml_text_edit_ctrl *self) { return self->is_auto_indenting(); },
        [](gui_ml_text_edit_ctrl *self, const bool value) { self->set_auto_indenting(value); });

    builder.property(
        "tabspaces",
        [](const gui_ml_text_edit_ctrl *self) { return self->get_tab_spaces(); },
        [](gui_ml_text_edit_ctrl *self, const int value) { self->set_tab_spaces(value); });

    builder.property(
        "syntaxhighlighting",
        [](const gui_ml_text_edit_ctrl *self) { return self->is_syntax_highlighting(); },
        [](gui_ml_text_edit_ctrl *self, const bool value) { self->set_syntax_highlighting(value); });

    builder.function("settext", [](gui_ml_text_edit_ctrl *self, const values &args) -> expected_value {
        self->set_text(text_at(args, 0));

        return value{};
    });

    builder.function("gettext", [](const gui_ml_text_edit_ctrl *self, const values &) -> expected_value {
        return value{self->get_text()};
    });

    builder.function("addtext", [](gui_ml_text_edit_ctrl *self, const values &args) -> expected_value {
        self->add_text(text_at(args, 0));

        return value{};
    });

    builder.function("getlinecount", [](const gui_ml_text_edit_ctrl *self, const values &) -> expected_value {
        return value{static_cast<double>(self->get_line_count())};
    });

    builder.function("getlines", [](const gui_ml_text_edit_ctrl *self, const values &) -> expected_value {
        auto lines = std::make_shared<array>();
        for (const auto &line : self->get_lines()) {
            lines->elements.emplace_back(line);
        }

        return value{lines};
    });

    builder.function("setlines", [](gui_ml_text_edit_ctrl *self, const values &args) -> expected_value {
        self->set_lines(to_lines(args));

        return value{};
    });

    builder.function("scrolltobottom", [](gui_ml_text_edit_ctrl *self, const values &) -> expected_value {
        self->scroll_to_bottom();

        return value{};
    });

    builder.function("scrolltotop", [](gui_ml_text_edit_ctrl *self, const values &) -> expected_value {
        self->scroll_to_top();

        return value{};
    });

    builder.function("reflow", [](gui_ml_text_edit_ctrl *, const values &) -> expected_value { return value{}; });

    env_.register_type(builder.build());
}

void script_host::bind_popup_menu_type() {
    auto builder = prototype_builder<gui_popup_menu_ctrl>("GuiPopUpMenuCtrl");

    builder.constructor([](const values &args) -> std::shared_ptr<void> {
        return adopt<gui_popup_menu_ctrl>(args, "guiblue_button.png");
    });
    with_common(builder);
    with_text(builder);

    builder.property(
        "textprofile",
        [](const gui_popup_menu_ctrl *) { return std::string{}; },
        [](gui_popup_menu_ctrl *self, const std::string &value) {
            if (auto profile = get_script_host().find_profile(value)) {
                self->set_text_profile(profile);
            }
        });

    builder.property(
        "scrollprofile",
        [](const gui_popup_menu_ctrl *) { return std::string{}; },
        [](gui_popup_menu_ctrl *, const std::string &) {});

    builder.property(
        "maxpopupheight",
        [](const gui_popup_menu_ctrl *) { return 0.0f; },
        [](gui_popup_menu_ctrl *self, const float value) { self->set_max_popup_height(value); });

    builder.function("clearrows", [](gui_popup_menu_ctrl *self, const values &) -> expected_value {
        self->clear_rows();

        return value{};
    });

    builder.function("addrow", [](gui_popup_menu_ctrl *self, const values &args) -> expected_value {
        self->add_row(static_cast<int>(number_at(args, 0)), text_at(args, 1));

        return value{};
    });

    builder.function("insertrow", [](gui_popup_menu_ctrl *self, const values &args) -> expected_value {
        self->insert_row(static_cast<int>(number_at(args, 0)), static_cast<int>(number_at(args, 1)), text_at(args, 2));

        return value{};
    });

    builder.function("removerow", [](gui_popup_menu_ctrl *self, const values &args) -> expected_value {
        self->remove_row(static_cast<int>(number_at(args, 0)));

        return value{};
    });

    builder.function("clearselection", [](gui_popup_menu_ctrl *self, const values &) -> expected_value {
        self->clear_selection();

        return value{};
    });

    builder.function("setselectedrow", [](gui_popup_menu_ctrl *self, const values &args) -> expected_value {
        self->set_selected_index(static_cast<int>(number_at(args, 0)));

        return value{};
    });

    builder.function("setselectedbyid", [](gui_popup_menu_ctrl *self, const values &args) -> expected_value {
        self->set_selected_id(static_cast<int>(number_at(args, 0)));

        return value{};
    });

    builder.function("setselected", [](gui_popup_menu_ctrl *self, const values &args) -> expected_value {
        self->set_selected_index(static_cast<int>(number_at(args, 0)));

        return value{};
    });

    builder.function("getselectedrow", [](const gui_popup_menu_ctrl *self, const values &) -> expected_value {
        return value{static_cast<double>(self->get_selected_index())};
    });

    builder.function("getselected", [](const gui_popup_menu_ctrl *self, const values &) -> expected_value {
        return value{static_cast<double>(self->get_selected_index())};
    });

    builder.function("getselectedid", [](const gui_popup_menu_ctrl *self, const values &) -> expected_value {
        return value{static_cast<double>(self->get_selected_id())};
    });

    builder.function("getselectedtext", [](const gui_popup_menu_ctrl *self, const values &) -> expected_value {
        return value{self->get_selected_text()};
    });

    builder.function("rowcount", [](const gui_popup_menu_ctrl *self, const values &) -> expected_value {
        return value{static_cast<double>(self->get_rows().size())};
    });

    builder.function("findtext", [](const gui_popup_menu_ctrl *self, const values &args) -> expected_value {
        const auto text = text_at(args, 0);
        const auto &rows = self->get_rows();

        for (size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].text == text) {
                return value{static_cast<double>(i)};
            }
        }

        return value{-1.0};
    });

    builder.function("open", [](gui_popup_menu_ctrl *self, const values &) -> expected_value {
        self->open();

        return value{};
    });

    builder.function("close", [](gui_popup_menu_ctrl *self, const values &) -> expected_value {
        self->close();

        return value{};
    });

    builder.function("forceclose", [](gui_popup_menu_ctrl *self, const values &) -> expected_value {
        self->close();

        return value{};
    });

    builder.function("sort", [](gui_popup_menu_ctrl *, const values &) -> expected_value { return value{}; });

    env_.register_type(builder.build());
}

void script_host::bind_tree_view_types() {
    {
        auto builder = prototype_builder<gui_tree_view_ctrl::node>("GuiTreeViewNode");

        builder.property("name", [](const gui_tree_view_ctrl::node *self) { return self->name; });
        builder.property("level", [](const gui_tree_view_ctrl::node *self) { return self->depth(); });
        builder.property(
            "expanded",
            [](const gui_tree_view_ctrl::node *self) { return self->expanded; },
            [](gui_tree_view_ctrl::node *self, const bool value) { self->expanded = value; });
        builder.property(
            "sortgroup",
            [](const gui_tree_view_ctrl::node *self) { return self->sort_group; },
            [](gui_tree_view_ctrl::node *self, const int value) { self->sort_group = value; });
        builder.function("getpath", [](const gui_tree_view_ctrl::node *self, const values &args) -> expected_value {
            const auto separator = text_at(args, 0);

            return value{self->path(separator.empty() ? '/' : separator.front())};
        });

        tree_node_proto_ = builder.build();
        env_.register_type(tree_node_proto_);
    }

    auto builder = prototype_builder<gui_tree_view_ctrl>("GuiTreeViewCtrl");

    builder.constructor([](const values &args) -> std::shared_ptr<void> {
        return adopt<gui_tree_view_ctrl>(args, "guiblue_scroll.png");
    });
    with_common(builder);

    builder.property(
        "textprofile",
        [](const gui_tree_view_ctrl *) { return std::string{}; },
        [](gui_tree_view_ctrl *self, const std::string &value) {
            if (auto profile = get_script_host().find_profile(value)) {
                self->set_text_profile(profile);
            }
        });

    builder.property(
        "columns",
        [](const gui_tree_view_ctrl *) { return std::string{}; },
        [](gui_tree_view_ctrl *self, const std::string &value) { self->set_columns(to_offsets(value)); });

    for (const auto *name : {"fitparentwidth", "clipcolumntext", "expandondoubleclick"}) {
        builder.property(
            name,
            [](const gui_tree_view_ctrl *) { return false; },
            [](gui_tree_view_ctrl *, const bool) {});
    }

    for (const auto *name : {"sortmode", "sortorder", "groupsortorder"}) {
        builder.property(
            name,
            [](const gui_tree_view_ctrl *) { return std::string{}; },
            [](gui_tree_view_ctrl *, const std::string &) {});
    }

    builder.function("clearnodes", [](gui_tree_view_ctrl *self, const values &) -> expected_value {
        self->clear_nodes();

        return value{};
    });
    builder.function("addnode", [](gui_tree_view_ctrl *self, const values &args) -> expected_value {
        return get_script_host().wrap_tree_node(self->add_node(text_at(args, 0)));
    });
    builder.function("addnodebypath", [](gui_tree_view_ctrl *self, const values &args) -> expected_value {
        return get_script_host().wrap_tree_node(self->add_node_by_path(text_at(args, 0), text_at(args, 1)));
    });
    builder.function("getnodebypath", [](const gui_tree_view_ctrl *self, const values &args) -> expected_value {
        return get_script_host().wrap_tree_node(self->get_node_by_path(text_at(args, 0), text_at(args, 1)));
    });
    builder.function("getnode", [](const gui_tree_view_ctrl *self, const values &args) -> expected_value {
        return get_script_host().wrap_tree_node(self->get_node_by_path(text_at(args, 0), "/"));
    });
    builder.function("getselectednode", [](const gui_tree_view_ctrl *self, const values &) -> expected_value {
        return get_script_host().wrap_tree_node(self->get_selected_node());
    });
    builder.function("sort", [](gui_tree_view_ctrl *self, const values &) -> expected_value {
        self->sort();

        return value{};
    });
    builder.function("seticonsize", [](gui_tree_view_ctrl *, const values &) -> expected_value { return value{}; });

    env_.register_type(builder.build());
}

auto script_host::wrap_tree_node(const std::shared_ptr<gui_tree_view_ctrl::node> &node) const -> value {
    return node ? value{std::make_shared<prototype_dictionary>(tree_node_proto_, node)} : value{};
}

void script_host::clear() {
    for (const auto &[name, control] : controls_) {
        if (!control) {
            continue;
        }

        control->set_visible(false);
        control->clear_controls();

        if (const auto parent = control->get_parent()) {
            parent->remove_child(control);
        }
    }

    weapons_.clear();
    shown_.clear();
    controls_.clear();
    pending_events_.clear();
    env_.clear_objects();
    default_movement_ = true;
    screen_effect_ = Color{0, 0, 0, 0};

    bind_builtin_objects();
}

void script_host::set_show_entry(show_entry entry) {
    const auto index = entry.index;

    shown_[index] = std::make_shared<show_entry>(std::move(entry));
}

void script_host::hide_show_entry(const int index) {
    shown_.erase(index);
}

auto script_host::find_show_entry(const int index) -> const show_entry_ptr & {
    auto &entry = shown_[index];
    if (!entry) {
        entry = std::make_shared<show_entry>();
        entry->index = index;
        entry->emitter = std::make_shared<prototype_dictionary>(emitter_proto_, std::make_shared<particle_emitter>());
    }

    return entry;
}

void script_host::bind() {
    for (const auto *name : {"echo", "trace"}) {
        env_.register_function(name, [](environment &, const values &args) -> expected_value {
            TraceLog(LOG_INFO, "[script] %s", text_at(args, 0).c_str());

            return {};
        });
    }

    env_.register_function("showtext", [this](environment &, const values &args) -> expected_value {
        const auto &entry = find_show_entry(static_cast<int>(number_at(args, 0)));

        entry->text = text_at(args, 5);
        entry->position = {static_cast<float>(number_at(args, 1)), static_cast<float>(number_at(args, 2))};
        entry->font = text_at(args, 3);
        entry->style = text_at(args, 4);
        entry->image.clear();

        return {};
    });

    env_.register_function("showimg", [this](environment &, const values &args) -> expected_value {
        const auto &entry = find_show_entry(static_cast<int>(number_at(args, 0)));

        entry->image = text_at(args, 1);
        entry->position = {static_cast<float>(number_at(args, 2)), static_cast<float>(number_at(args, 3))};

        return {};
    });

    env_.register_function("worldx", [](environment &, const values &args) -> expected_value {
        const auto [x, y] = screen_to_world({static_cast<float>(number_at(args, 0)), static_cast<float>(number_at(args, 1))});

        return value{static_cast<double>(x / tile_size)};
    });

    env_.register_function("worldy", [](environment &, const values &args) -> expected_value {
        const auto [x, y] = screen_to_world({static_cast<float>(number_at(args, 0)), static_cast<float>(number_at(args, 1))});

        return value{static_cast<double>(y / tile_size)};
    });

    env_.register_function("getz", [](environment &, const values &) -> expected_value {
        return value{0.0};
    });

    env_.register_function("gettextwidth", [](environment &, const values &args) -> expected_value {
        const auto size = base_text_size * static_cast<float>(number_at(args, 0));
        const auto text = text_at(args, 3);
        const auto font = load_font(text_at(args, 2).contains('b') ? "LiberationSans-Bold.ttf" : "LiberationSans-Regular.ttf", size);

        return value{static_cast<double>(MeasureTextEx(font, text.c_str(), size, 1).x)};
    });

    env_.register_function("addtiledef", [](environment &, const values &args) -> expected_value {
        add_tile_def(text_at(args, 0), text_at(args, 1),
            args.size() > 2 ? static_cast<int>(number_at(args, 2)) : og::shared::tileset_format_pics1);
        refresh_tileset();

        return {};
    });

    env_.register_function("removetiledefs", [](environment &, const values &) -> expected_value {
        clear_tile_defs();
        refresh_tileset();

        return {};
    });

    env_.register_function("loadmap", [](environment &, const values &) -> expected_value {
        return {};
    });

    env_.register_function("play", [](environment &, const values &args) -> expected_value {
        if (const auto file = text_at(args, 0); !file.empty() && !file.ends_with(".mid")) {
            play_sound(file);
        }

        return {};
    });

    for (const auto *name : {"sendtext", "requesttext", "stopmidi", "stopsound", "playlooped"}) {
        env_.register_function(name, [](environment &, const values &) -> expected_value {
            return {};
        });
    }

    env_.register_function("makefirstresponder", [](environment &, const values &args) -> expected_value {
        if (args.empty() || to_bool(args[0])) {
            get_chat_bar().close();
        }

        return value{};
    });

    env_.register_function("serverwarp", [](environment &, const values &args) -> expected_value {
        auto &network = get_network();

        const auto host = args.empty() ? network.get_host() : to_string(args[0]);
        const auto port = args.size() > 1 ? static_cast<uint16_t>(to_number(args[1])) : network.get_port();

        network.warp_to(host, port, network.get_token());

        return {};
    });

    env_.register_function("seteffect", [this](environment &, const values &args) -> expected_value {
        const auto channel = [&args](const size_t index) {
            return static_cast<unsigned char>(std::clamp(number_at(args, index), 0.0, 1.0) * 255.0);
        };

        screen_effect_ = Color{channel(0), channel(1), channel(2), channel(3)};

        return {};
    });

    for (const auto *name : {"showstats", "enableFeatures", "disablepause", "disableselectweapons"}) {
        env_.register_function(name, [](environment &, const values &) -> expected_value {
            return {};
        });
    }

    env_.register_function("destroy", [this](environment &, const values &) -> expected_value {
        if (active_ != nullptr) {
            active_->destroyed = true;
        }

        return std::unexpected(error{
            .source = error_source::interpreter,
            .kind = error_kind::halted,
            .message = "destroyed",
        });
    });

    env_.register_function("setani", [](environment &, const values &args) -> expected_value {
        if (auto *local = get_local_player()) {
            local->set_animation(text_at(args, 0));
        }

        return {};
    });

    env_.register_function("tiletype", [](environment &, const values &args) -> expected_value {
        return value{static_cast<double>(get_tile_type(
            static_cast<int>(number_at(args, 0) * tile_size),
            static_cast<int>(number_at(args, 1) * tile_size)))};
    });

    env_.register_function("onwall", [](environment &, const values &args) -> expected_value {
        return value{on_wall(Vector2{
                         static_cast<float>(number_at(args, 0)) * tile_size,
                         static_cast<float>(number_at(args, 1)) * tile_size})
                         ? 1.0
                         : 0.0};
    });

    env_.register_function("onwall2", [](environment &, const values &args) -> expected_value {
        return value{on_wall(Rectangle{
                         static_cast<float>(number_at(args, 0)) * tile_size,
                         static_cast<float>(number_at(args, 1)) * tile_size,
                         static_cast<float>(number_at(args, 2)) * tile_size,
                         static_cast<float>(number_at(args, 3)) * tile_size})
                         ? 1.0
                         : 0.0};
    });

    env_.register_function("onwater2", [](environment &, const values &args) -> expected_value {
        const auto x = static_cast<int>(number_at(args, 0));
        const auto y = static_cast<int>(number_at(args, 1));
        const auto width = std::max(1, static_cast<int>(number_at(args, 2)));
        const auto height = std::max(1, static_cast<int>(number_at(args, 3)));

        for (auto row = y; row < y + height; ++row) {
            for (auto column = x; column < x + width; ++column) {
                if ((get_tile_type(column * tile_size, row * tile_size) & static_cast<int>(tile_type::water)) != 0) {
                    return value{1.0};
                }
            }
        }

        return value{0.0};
    });

    env_.register_function("findimg", [this](environment &, const values &args) -> expected_value {
        return value{std::make_shared<prototype_dictionary>(show_proto_, find_show_entry(static_cast<int>(number_at(args, 0))))};
    });

    env_.register_function("changeimgcolors", [this](environment &, const values &args) -> expected_value {
        const auto &entry = find_show_entry(static_cast<int>(number_at(args, 0)));

        entry->red = static_cast<float>(number_at(args, 1));
        entry->green = static_cast<float>(number_at(args, 2));
        entry->blue = static_cast<float>(number_at(args, 3));
        entry->alpha = static_cast<float>(number_at(args, 4));

        return {};
    });

    env_.register_function("changeimgvis", [this](environment &, const values &args) -> expected_value {
        find_show_entry(static_cast<int>(number_at(args, 0)))->visible = number_at(args, 1) > 0.0;

        return {};
    });

    env_.register_function("changeimgzoom", [this](environment &, const values &args) -> expected_value {
        find_show_entry(static_cast<int>(number_at(args, 0)))->zoom = static_cast<float>(number_at(args, 1));

        return {};
    });

    env_.register_function("hideimg", [this](environment &, const values &args) -> expected_value {
        hide_show_entry(static_cast<int>(number_at(args, 0)));

        return {};
    });

    env_.register_function("hideimgs", [this](environment &, const values &) -> expected_value {
        shown_.clear();

        return {};
    });

    env_.register_function("disabledefmovement", [this](environment &, const values &) -> expected_value {
        default_movement_ = false;

        return {};
    });

    env_.register_function("enabledefmovement", [this](environment &, const values &) -> expected_value {
        default_movement_ = true;

        return {};
    });

    env_.register_function("keydown", [](environment &, const values &args) -> expected_value {
        static constexpr std::array keys = {KEY_UP, KEY_LEFT, KEY_DOWN, KEY_RIGHT, KEY_D, KEY_S, KEY_A, KEY_M};

        const auto index = static_cast<size_t>(number_at(args, 0));

        return value{index < keys.size() && dev_input::key_down(keys[index]) ? 1.0 : 0.0};
    });

    for (const auto *name : {"keydown2", "keydown2global"}) {
        env_.register_function(name, [](environment &, const values &args) -> expected_value {
            const auto key = key_codes::to_raylib(static_cast<int>(number_at(args, 0)));

            return value{key != 0 && dev_input::key_down(key) ? 1.0 : 0.0};
        });
    }

    env_.register_function("getkeycode", [](environment &, const values &args) -> expected_value {
        return value{static_cast<double>(key_codes::of_name(text_at(args, 0)))};
    });

    env_.register_function("keyname", [](environment &, const values &args) -> expected_value {
        return value{key_codes::name_of(static_cast<int>(number_at(args, 0)))};
    });

    env_.register_function("setTimer", [this](environment &, const values &args) -> expected_value {
        if (active_ != nullptr) {
            active_->timer = number_at(args, 0);
        }

        return {};
    });

    env_.register_function("findcontrol", [this](environment &, const values &args) -> expected_value {
        if (auto control = find_control(text_at(args, 0))) {
            return value{static_cast<double>(control->is_visible() ? 1 : 0)};
        }

        return value{};
    });

    env_.register_function("removecontrol", [this](environment &, const values &args) -> expected_value {
        destroy_control(text_at(args, 0));

        return {};
    });

    env_.register_function("addcontrol", [this](environment &, const values &args) -> expected_value {
        auto parent = find_control(text_at(args, 0));
        auto child = find_control(text_at(args, 1));

        if (parent && child) {
            parent->add_child(child);
        }

        return {};
    });

    env_.register_function("catchevent", [this](environment &, const values &args) -> expected_value {
        catch_event(active_, text_at(args, 0), text_at(args, 1), text_at(args, 2));

        return {};
    });

    env_.register_function("triggerserver", [this](environment &, const values &args) -> expected_value {
        if (!trigger_sink_) {
            return {};
        }

        std::vector<std::string> extra;
        for (size_t i = 3; i < args.size(); ++i) {
            extra.push_back(to_string(args[i]));
        }

        trigger_sink_(text_at(args, 1), text_at(args, 2), extra);

        return {};
    });

    env_.register_function("triggeraction", [this](environment &, const values &args) -> expected_value {
        const auto action = text_at(args, 2);
        const auto first_param = is_name(action, "serverside") ? 5u : 3u;

        std::vector<std::string> extra;
        for (auto i = first_param; i < args.size(); ++i) {
            extra.push_back(to_string(args[i]));
        }

        if (is_name(action, "serverside")) {
            if (trigger_sink_) {
                trigger_sink_(text_at(args, 3), text_at(args, 4), extra);
            }

            return {};
        }

        if (action_sink_) {
            action_sink_(static_cast<float>(number_at(args, 0)), static_cast<float>(number_at(args, 1)), action, extra);
        }

        return {};
    });
}

namespace {
    auto make_player_entry(const uint16_t id, const actor &who, const std::string &nick) -> dictionary_ptr {
        constexpr size_t gani_attr_count = 31;

        auto entry = std::make_shared<basic_dictionary>();
        const auto [x, y] = who.get_position();

        entry->put("id", value{static_cast<double>(id)});
        entry->put("x", value{static_cast<double>(x / tile_size)});
        entry->put("y", value{static_cast<double>(y / tile_size)});
        entry->put("z", value{0.0});
        entry->put("dir", value{static_cast<double>(who.get_direction())});
        entry->put("ani", value{who.get_animation()});
        entry->put("nick", value{nick});
        entry->put("attr", value{std::make_shared<array>(values(gani_attr_count))});

        return entry;
    }
}

void script_host::publish_players() {
    auto list = std::make_shared<array>();

    if (const auto *local = get_local_player()) {
        list->elements.emplace_back(make_player_entry(get_network().get_own_id(), *local, get_network().get_account()));
    }

    for (const auto &[id, remote] : get_network().get_players()) {
        list->elements.emplace_back(make_player_entry(id, remote, remote.get_nickname()));
    }

    env_.put("players", value{list});
}

void script_host::publish_globals() {
    if (!player_object_) {
        if (auto *local = get_local_player()) {
            player_object_ = std::make_shared<prototype_dictionary>(
                player_proto_, std::shared_ptr<player>(std::shared_ptr<void>(), local));
        }
    }

    if (player_object_) {
        player_object_->put("chat", value{last_chat_});
        player_object_->put("account", value{get_network().get_account()});
        player_object_->put("client", value{client_flags_});
        player_object_->put("clientr", value{clientr_flags_});

        auto level = std::make_shared<basic_dictionary>();

        level->put("name", value{get_current_level() ? get_current_level()->get_name() : std::string{}});

        player_object_->put("level", value{level});

        env_.put("player", value{player_object_});
    }

    env_.put("screenwidth", value{static_cast<double>(GetScreenWidth())});
    env_.put("screenheight", value{static_cast<double>(GetScreenHeight())});

    const auto [mouse_x, mouse_y] = screen_to_world(dev_input::mouse_position());

    env_.put("mousex", value{static_cast<double>(mouse_x / tile_size)});
    env_.put("mousey", value{static_cast<double>(mouse_y / tile_size)});

    publish_players();

    env_.put("client", value{client_flags_});
    env_.put("clientr", value{clientr_flags_});
    env_.put("server", value{server_flags_});
    env_.put("serverr", value{server_flags_});
}

void script_host::add_weapon(const std::string &name, const std::string &image, const std::string &source) {
    if (source.find_first_not_of(" \t\r\n") == std::string::npos) {
        return;
    }

    std::istringstream is(source);

    auto parsed = load_script(env_, is);
    if (!parsed) {
        TraceLog(LOG_ERROR, "SCRIPT: %s: %s (line %d)",
            name.c_str(), parsed.error().message.c_str(), parsed.error().position.line);

        return;
    }

    (*parsed)->set_name(name);

    auto weapon = std::make_unique<client_weapon>();
    weapon->name = name;
    weapon->image = image;
    weapon->script = *parsed;

    auto *added = weapon.get();
    added->self = std::make_shared<script_object>(*this, *added);

    env_.register_object(name, added->self);

    weapons_.push_back(std::move(weapon));

    TraceLog(LOG_INFO, "SCRIPT: loaded weapon '%s'", name.c_str());
}

void script_host::remove_weapon(const std::string &name) {
    if (auto *weapon = find_weapon(name)) {
        weapon->destroyed = true;
    }
}

void script_host::ensure_created(client_weapon &weapon) {
    if (weapon.created) {
        return;
    }

    weapon.created = true;

    dispatch(weapon, "onCreated", {});
}

void script_host::dispatch(client_weapon &weapon, const std::string_view event, const values &args) {
    if (!weapon.script || !weapon.script->has_function(event)) {
        return;
    }

    publish_globals();

    auto *previous = std::exchange(active_, &weapon);

    const auto result = weapon.script->call_with(event, weapon.self, weapon.locals, args);

    active_ = previous;

    if (!result && result.error().kind != error_kind::halted) {
        TraceLog(LOG_ERROR, "SCRIPT: %s.%s: %s (line %d)",
            weapon.name.c_str(),
            std::string(event).c_str(),
            result.error().message.c_str(),
            result.error().position.line);
    }
}

void script_host::update(const float dt) {
    for (const auto &weapon : weapons_) {
        ensure_created(*weapon);
    }

    const auto events = std::exchange(pending_events_, {});

    for (const auto &[owner, handler, args] : events) {
        if (owner->script && owner->script->has_function(handler)) {
            dispatch(*owner, handler, args);
        }
    }

    if (flag_sink_) {
        for (const auto &name : client_flags_->take_changed()) {
            flag_sink_(name, to_string(client_flags_->get(name)));
        }
    }

    for (const auto &sync : show_img_actors_) {
        sync();
    }

    for (const auto &weapon : weapons_) {
        if (weapon->timer <= 0.0) {
            continue;
        }

        weapon->timer -= dt;

        if (weapon->timer <= 0.0) {
            weapon->timer = 0.0;

            dispatch(*weapon, "onTimeout", {});
        }
    }

    std::erase_if(weapons_, [this](const std::unique_ptr<client_weapon> &weapon) {
        if (!weapon->destroyed) {
            return false;
        }

        env_.erase_object(weapon->name);
        std::erase_if(pending_events_, [&weapon](const pending_event &event) { return event.owner == weapon.get(); });

        return true;
    });
}

void script_host::set_flag(const og::net::flag_store store, const std::string &name, const std::string &text) {
    switch (store) {
    case og::net::flag_store::serverr:
        server_flags_->put(name, value{text});
        break;

    case og::net::flag_store::clientr:
        clientr_flags_->put(name, value{text});
        break;

    case og::net::flag_store::client:
        client_flags_->accept(name, value{text});
        break;
    }
}

void script_host::player_entered_level() {
    for (const auto &weapon : weapons_) {
        dispatch(*weapon, "onPlayerEnters", {});
    }
}

void script_host::player_chatted(const std::string &text) {
    last_chat_ = text;

    for (const auto &weapon : weapons_) {
        dispatch(*weapon, "onPlayerChats", {value{text}});
    }
}

void script_host::key_pressed(const int keycode) {
    const auto name = key_codes::name_of(keycode);
    const auto character = name.size() == 1 ? name : std::string{};

    for (const auto &weapon : weapons_) {
        dispatch(*weapon, "onKeyPressed", {value{static_cast<double>(keycode)}, value{character}});
    }
}

void script_host::action(const std::string &name, const std::string &action, const std::vector<std::string> &args) {
    values arguments{value{action}};
    for (const auto &argument : args) {
        arguments.emplace_back(argument);
    }

    for (const auto &weapon : weapons_) {
        if (weapon->name == name) {
            ensure_created(*weapon);
            dispatch(*weapon, "onActionClientside", arguments);
        }
    }
}

void script_host::draw() const {
    std::vector<const show_entry *> ordered;
    ordered.reserve(shown_.size());

    for (const auto &[index, entry] : shown_) {
        if (entry && entry->visible) {
            ordered.push_back(entry.get());
        }
    }

    std::ranges::stable_sort(ordered, {}, [](const show_entry *entry) { return entry->layer; });

    for (const auto *entry : ordered) {
        const auto tint = to_color(*entry);

        const auto position = world_to_screen({entry->position.x * tile_size, entry->position.y * tile_size});

        if (!entry->image.empty()) {
            if (const auto texture = load_texture(entry->image); IsTextureValid(texture)) {
                DrawTextureV(texture, position, tint);
            }

            continue;
        }

        if (entry->text.empty()) {
            continue;
        }

        const auto size = base_text_size * entry->zoom;
        const auto font = load_font(entry->style.contains('b') ? "LiberationSans-Bold.ttf" : "LiberationSans-Regular.ttf", size);

        DrawTextEx(font, entry->text.c_str(), {position.x + 1, position.y + 1}, size, 1, Fade(BLACK, 0.7f));
        DrawTextEx(font, entry->text.c_str(), position, size, 1, tint);
    }
}

void script_host::register_control(const std::string &name, const std::shared_ptr<gui_control> &control) {
    if (name.empty() || !control) {
        return;
    }

    if (const auto existing = controls_.find(name); existing != controls_.end() && existing->second) {
        existing->second->set_visible(false);
        existing->second->clear_controls();

        if (const auto parent = existing->second->get_parent()) {
            parent->remove_child(existing->second);
        }
    }

    controls_[name] = control;
}

void script_host::destroy_control(const std::string &name) {
    const auto control = find_control(name);
    if (!control) {
        return;
    }

    control->set_visible(false);
    control->clear_controls();

    if (const auto parent = control->get_parent()) {
        parent->remove_child(control);
    }

    controls_.erase(name);
    env_.erase_object(name);
}

auto script_host::find_control(const std::string &name) const -> std::shared_ptr<gui_control> {
    const auto match = controls_.find(name);

    return match == controls_.end() ? nullptr : match->second;
}

void script_host::register_instance(const std::shared_ptr<gui_control> &control) {
    if (control) {
        instances_[control.get()] = control;
    }
}

auto script_host::find_instance(const dictionary_ptr &object) const -> std::shared_ptr<gui_control> {
    const auto prototype = std::dynamic_pointer_cast<prototype_dictionary>(object);
    if (!prototype) {
        return nullptr;
    }

    const auto match = instances_.find(prototype->get_instance().get());

    return match == instances_.end() ? nullptr : match->second.lock();
}

void script_host::register_profile(const std::string &name, const std::shared_ptr<gui_control_profile> &profile) {
    if (!name.empty() && profile) {
        profiles_[name] = profile;
    }
}

auto script_host::find_profile(const std::string &name) const -> std::shared_ptr<gui_control_profile> {
    const auto match = profiles_.find(name);

    return match == profiles_.end() ? nullptr : match->second;
}

void script_host::bind_builtin_objects() {
    auto builder = prototype_builder<script_host>("GraalControl");

    builder.function("isfirstresponder", [](const script_host *, const values &) -> expected_value {
        return value{get_chat_bar().is_open() ? 0.0 : 1.0};
    });
    builder.function("makefirstresponder", [](script_host *, const values &args) -> expected_value {
        if (args.empty() || to_bool(args[0])) {
            get_chat_bar().close();
        }

        return value{};
    });

    env_.register_object(
        "GraalControl",
        std::make_shared<prototype_dictionary>(builder.build(), std::shared_ptr<script_host>(std::shared_ptr<void>(), this)));

    if (const auto &input = get_chat_bar().get_input()) {
        register_instance(input);

        if (auto proto = env_.find_type("GuiTextEditCtrl")) {
            env_.register_object("ChatBar", std::make_shared<prototype_dictionary>(std::move(proto), input));
        }
    }
}

auto script_host::call_weapon(client_weapon &weapon, const std::string &function, const values &args) -> expected_value {
    if (!weapon.script) {
        return value{};
    }

    auto *previous = std::exchange(active_, &weapon);
    auto result = weapon.script->call_with(function, weapon.self, weapon.locals, args);

    active_ = previous;

    return result;
}

void script_host::queue_event(client_weapon *owner, const std::string &handler, values args) {
    if (owner != nullptr) {
        pending_events_.emplace_back(owner, handler, std::move(args));
    }
}

auto script_host::find_weapon(const std::string &name) -> client_weapon * {
    for (const auto &weapon : weapons_) {
        if (weapon->name == name && !weapon->destroyed) {
            return weapon.get();
        }
    }

    return nullptr;
}

void script_host::catch_event(client_weapon *owner, const std::string &control_name, const std::string &event, const std::string &handler) {
    auto control = find_control(control_name);
    if (owner == nullptr || !control || handler.empty()) {
        return;
    }

    const auto weapon_name = owner->name;

    if (is_name(event, "onAction")) {
        control->clicked = [this, weapon_name, handler, control_name] {
            queue_event(find_weapon(weapon_name), handler, {value{control_name}});
        };

        return;
    }

    if (is_name(event, "onMouseDown")) {
        control->mouse_pressed = [this, weapon_name, handler, control_name] {
            queue_event(find_weapon(weapon_name), handler, {value{control_name}});
        };

        return;
    }

    auto report = [this, weapon_name, handler, control_name](const int id, const std::string &text, const int index) {
        queue_event(find_weapon(weapon_name), handler,
            {value{control_name}, value{static_cast<double>(id)}, value{text}, value{static_cast<double>(index)}});
    };

    if (auto tabs = std::dynamic_pointer_cast<gui_tab_ctrl>(control)) {
        if (is_name(event, "onSelect")) {
            tabs->selected = report;
        } else if (is_name(event, "onDeselect")) {
            tabs->deselected = report;
        }

        return;
    }

    if (auto list = std::dynamic_pointer_cast<gui_text_list_ctrl>(control)) {
        if (is_name(event, "onSelect")) {
            list->selected = report;
        }

        return;
    }

    if (auto menu = std::dynamic_pointer_cast<gui_popup_menu_ctrl>(control)) {
        if (is_name(event, "onSelect")) {
            menu->selected = report;
        }

        return;
    }

    if (auto tree = std::dynamic_pointer_cast<gui_tree_view_ctrl>(control)) {
        if (is_name(event, "onSelect")) {
            tree->selected = [this, weapon_name, handler, control_name](
                                 const gui_tree_view_ctrl::node_ptr &node, const std::string &slash, const std::string &dot) {
                queue_event(find_weapon(weapon_name), handler,
                    {value{control_name}, wrap_tree_node(node), value{slash}, value{dot}});
            };
        }
    }
}

void script_host::attach_control(const std::shared_ptr<gui_control> &control) const {
    if (gui_root_ && control) {
        gui_root_->add_child(control);
    }
}
