#pragma once

#include <gs2/basic_dictionary.hpp>
#include <gs2/environment.hpp>
#include <gs2/prototype.hpp>
#include <gs2/script.hpp>
#include <net/message.hpp>

#include <functional>
#include <map>
#include <memory>
#include "gui/gui_control.hpp"
#include "gui/gui_tree_view_ctrl.hpp"

#include <raylib.h>
#include <string>
#include <vector>

struct show_entry {
    int index = 0;
    int layer = 0;
    std::string text;
    std::string image;
    std::string font;
    std::string style;
    Vector2 position{};
    float zoom = 1.0f;
    float red = 1.0f;
    float green = 1.0f;
    float blue = 1.0f;
    float alpha = 1.0f;
    bool visible = true;

    og::gs2::dictionary_ptr emitter;
};

using show_entry_ptr = std::shared_ptr<show_entry>;

class script_host;
struct client_weapon;

class script_object : public og::gs2::basic_dictionary {
  public:
    script_object(script_host &host, client_weapon &owner) : host_(&host), owner_(&owner) {
    }

    auto contains(std::string_view name) -> bool override;
    auto get(std::string_view name) -> og::gs2::value override;

  private:
    [[nodiscard]] auto has_function(std::string_view name) const -> bool;

    script_host *host_;
    client_weapon *owner_;
};

struct client_weapon {
    std::string name;
    std::string image;
    og::gs2::script_ptr script;
    og::gs2::dictionary_ptr self;
    og::gs2::dictionary_ptr locals = std::make_shared<og::gs2::basic_dictionary>();
    double timer = 0.0;
    bool created = false;
    bool destroyed = false;
};

class script_host {
  public:
    using trigger_sink = std::function<void(const std::string &name, const std::string &action, const std::vector<std::string> &args)>;
    using action_sink = std::function<void(float x, float y, const std::string &action, const std::vector<std::string> &args)>;
    using flag_sink = std::function<void(const std::string &name, const std::string &text)>;

    script_host();

    void on_trigger_server(trigger_sink sink) { trigger_sink_ = std::move(sink); }
    void on_trigger_action(action_sink sink) { action_sink_ = std::move(sink); }
    void on_client_flag(flag_sink sink) { flag_sink_ = std::move(sink); }

    void add_weapon(const std::string &name, const std::string &image, const std::string &source);
    void remove_weapon(const std::string &name);

    void clear();

    void update(float dt);
    void draw() const;

    void key_pressed(int key);

    void player_entered_level();
    void player_chatted(const std::string &text);
    void set_flag(og::net::flag_store store, const std::string &name, const std::string &text);
    void action(const std::string &name, const std::string &action, const std::vector<std::string> &args);

    [[nodiscard]] auto is_default_movement_enabled() const -> bool { return default_movement_; }

    [[nodiscard]] auto get_screen_effect() const -> Color { return screen_effect_; }
    [[nodiscard]] auto get_weapon_count() const -> size_t { return weapons_.size(); }

    void set_show_entry(show_entry entry);
    void hide_show_entry(int index);

    [[nodiscard]]
    auto find_show_entry(int index) -> const show_entry_ptr &;

    void set_gui_root(const std::shared_ptr<gui_control> &root) { gui_root_ = root; }

    void register_control(const std::string &name, const std::shared_ptr<gui_control> &control);

    void destroy_control(const std::string &name);

    void attach_control(const std::shared_ptr<gui_control> &control) const;

    [[nodiscard]]
    auto find_control(const std::string &name) const -> std::shared_ptr<gui_control>;

    void register_instance(const std::shared_ptr<gui_control> &control);
    void register_profile(const std::string &name, const std::shared_ptr<gui_control_profile> &profile);

    [[nodiscard]]
    auto find_instance(const og::gs2::dictionary_ptr &object) const -> std::shared_ptr<gui_control>;

    [[nodiscard]]
    auto find_profile(const std::string &name) const -> std::shared_ptr<gui_control_profile>;

    void queue_event(client_weapon *owner, const std::string &handler, og::gs2::values args);
    void catch_event(client_weapon *owner, const std::string &control_name, const std::string &event, const std::string &handler);

    void bind_builtin_objects();

    auto call_weapon(client_weapon &weapon, const std::string &function, const og::gs2::values &args) -> og::gs2::expected_value;

    [[nodiscard]]
    auto wrap_tree_node(const gui_tree_view_ctrl::node_ptr &node) const -> og::gs2::value;

  private:
    struct pending_event {
        client_weapon *owner = nullptr;
        std::string handler;
        og::gs2::values args;
    };

    void bind();
    void bind_preferences();
    void bind_gui_types();
    void bind_ml_text_edit_type();
    void bind_popup_menu_type();
    void bind_tree_view_types();
    void bind_profile_types();
    void bind_show_type();
    void bind_emitter_type();
    void bind_player_type();
    void ensure_created(client_weapon &weapon);
    void dispatch(client_weapon &weapon, std::string_view event, const og::gs2::values &args);
    [[nodiscard]] auto find_weapon(const std::string &name) -> client_weapon *;
    void publish_globals();
    void publish_players();

    og::gs2::environment env_;

    class client_flag_store final : public og::gs2::basic_dictionary {
      public:
        void put(std::string_view name, og::gs2::value value) override;

        [[nodiscard]] auto take_changed() -> std::vector<std::string>;

        void accept(std::string_view name, og::gs2::value value);

      private:
        std::vector<std::string> changed_;
    };

    std::shared_ptr<client_flag_store> client_flags_ = std::make_shared<client_flag_store>();
    og::gs2::dictionary_ptr clientr_flags_ = std::make_shared<og::gs2::basic_dictionary>();
    og::gs2::dictionary_ptr server_flags_ = std::make_shared<og::gs2::basic_dictionary>();
    std::vector<std::unique_ptr<client_weapon>> weapons_;
    std::map<int, show_entry_ptr> shown_;
    og::gs2::prototype_ptr show_proto_;
    og::gs2::prototype_ptr player_proto_;
    og::gs2::prototype_ptr tree_node_proto_;
    og::gs2::prototype_ptr emitter_proto_;
    og::gs2::dictionary_ptr player_object_;
    trigger_sink trigger_sink_;
    action_sink action_sink_;
    flag_sink flag_sink_;
    client_weapon *active_ = nullptr;
    std::shared_ptr<gui_control> gui_root_;
    std::map<std::string, std::shared_ptr<gui_control>> controls_;
    std::map<std::string, std::shared_ptr<gui_control_profile>> profiles_;
    std::map<const void *, std::weak_ptr<gui_control>> instances_;
    std::vector<pending_event> pending_events_;
    std::vector<std::function<void()>> show_img_actors_;
    std::string last_chat_;
    Color screen_effect_{0, 0, 0, 0};
    bool default_movement_ = true;
};

[[nodiscard]] auto get_script_host() -> script_host &;
