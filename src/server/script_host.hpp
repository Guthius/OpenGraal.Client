#pragma once

#include <gs2/basic_dictionary.hpp>
#include <gs2/environment.hpp>
#include <gs2/script.hpp>
#include <net/connection.hpp>
#include <net/message.hpp>
#include <server/world.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace og::server {
    struct session;

    class tracked_dictionary : public gs2::basic_dictionary {
      public:
        void put(std::string_view name, gs2::value value) override;
        auto erase(std::string_view name) -> bool override;

        [[nodiscard]] auto take_changed() -> std::vector<std::string>;

      private:
        std::vector<std::string> changed_;
    };

    using tracked_dictionary_ptr = std::shared_ptr<tracked_dictionary>;

    struct npc {
        uint32_t id = 0;
        std::string level;
        std::string name;
        gs2::script_ptr script;
        gs2::dictionary_ptr self = std::make_shared<gs2::basic_dictionary>();
        gs2::dictionary_ptr locals = std::make_shared<gs2::basic_dictionary>();
        net::npc_state state;
        double timer = 0.0;
        bool created = false;
        bool dirty = true;
    };

    struct weapon {
        std::string name;
        std::string image;
        std::string client_script;
        gs2::script_ptr script;
    };

    class script_host {
      public:
        using chat_sink = std::function<void(const npc &source, const std::string &text)>;
        using trigger_sink = std::function<void(const net::connection_ptr &target, const std::string &name, const std::string &action, const gs2::values &args)>;
        using item_sink = std::function<void(const std::string &level, float x, float y, const std::string &name, const std::string &image)>;
        using warp_sink = std::function<void(const net::connection_ptr &target, const std::string &host, uint16_t port, const std::string &account)>;
        using weapon_sink = std::function<void(session &player, const std::string &name, bool granted)>;
        using level_sink = std::function<void(session &player, const std::string &level, float x, float y)>;

        using player_source = std::function<std::vector<session *>()>;

        explicit script_host(world &game_world);

        void load();

        [[nodiscard]] auto get_weapons() const -> const std::vector<weapon> & { return weapons_; }
        [[nodiscard]] auto get_npcs() const -> const std::vector<std::shared_ptr<npc>> & { return npcs_; }
        [[nodiscard]] auto npcs_in(const std::string &level) const -> std::vector<std::shared_ptr<npc>>;
        [[nodiscard]] auto find_weapon(std::string_view name) const -> const weapon *;

        void on_chat(chat_sink sink) { chat_sink_ = std::move(sink); }
        void on_trigger_client(trigger_sink sink) { trigger_sink_ = std::move(sink); }
        void on_put_item(item_sink sink) { item_sink_ = std::move(sink); }
        void on_server_warp(warp_sink sink) { warp_sink_ = std::move(sink); }
        void on_weapon_changed(weapon_sink sink) { weapon_sink_ = std::move(sink); }
        void on_set_level(level_sink sink) { level_sink_ = std::move(sink); }
        void on_list_players(player_source source) { player_source_ = std::move(source); }

        void ensure_level(const std::string &level_name);

        auto reset_level(const std::string &level_name) -> std::vector<uint32_t>;

        void tick(double dt);

        void player_online(session &player);
        void player_logged_in(session &player);
        void player_offline(session &player);
        void player_entered(session &player);
        void player_left(session &player);
        void player_chatted(session &player, const std::string &text);
        void trigger_server(session &player, const std::string &weapon_name, const std::string &action, const gs2::values &args);

        void trigger_action(session &player, const std::string &level_name, float x, float y, const std::string &action, const gs2::values &args);
        void item_taken(session &player, const std::string &item_name);

        void report(const gs2::error &failure) const;

        [[nodiscard]] auto get_server_flags() const -> const tracked_dictionary_ptr & { return server_flags_; }
        [[nodiscard]] auto take_changed_server_flags() -> std::vector<std::string>;

      private:
        void bind_types();
        auto make_script(const std::string &name, const std::string &source) -> gs2::script_ptr;
        auto make_player_object(session &player) -> gs2::dictionary_ptr;
        auto list_players() -> gs2::array_ptr;
        static void write_back_player(session &player);
        void dispatch(const std::shared_ptr<npc> &target, std::string_view event, const gs2::values &args, session *player);

        world &world_;
        gs2::environment env_;
        std::vector<weapon> weapons_;
        std::vector<std::shared_ptr<npc>> npcs_;
        std::vector<std::string> instantiated_levels_;
        tracked_dictionary_ptr server_flags_ = std::make_shared<tracked_dictionary>();
        chat_sink chat_sink_;
        trigger_sink trigger_sink_;
        item_sink item_sink_;
        warp_sink warp_sink_;
        weapon_sink weapon_sink_;
        level_sink level_sink_;
        player_source player_source_;
        npc *active_ = nullptr;
        session *active_player_ = nullptr;
        uint32_t next_npc_id_ = 1;
        double accumulator_ = 0.0;
    };
}
