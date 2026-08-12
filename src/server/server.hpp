#pragma once

#include <net/acceptor.hpp>
#include <net/connection.hpp>
#include <server/account.hpp>
#include <server/script_host.hpp>
#include <server/world.hpp>

#include <chrono>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace og::server {
    struct session {
        net::connection_ptr connection;
        net::player_state state;
        account profile;
        std::string level;
        std::string last_chat;
        bool authenticated = false;
        size_t creates_attempted = 0;
        bool editing = false;
        std::set<std::string> edit_levels;
        bool link_grace = false;
        tracked_dictionary_ptr client_flags = std::make_shared<tracked_dictionary>();
        tracked_dictionary_ptr clientr_flags = std::make_shared<tracked_dictionary>();
        std::vector<std::string> weapons;
        gs2::dictionary_ptr script_object;
        gs2::script_ptr classes;
        std::vector<std::string> joined_classes;
        gs2::array_ptr attr;
        bool state_changed = false;

        [[nodiscard]] auto identifier() const -> std::string;
    };

    struct level_item {
        net::item_state state;
        std::string level;
    };

    struct pending_warp {
        std::shared_ptr<session> player;
        std::string level;
        float x = 0.0f;
        float y = 0.0f;
    };

    struct server_options {
        std::filesystem::path world_path = "world";
        uint16_t port = 14900;
        bool allow_registration = true;
        size_t max_players = 64;
    };

    class server {
      public:
        explicit server(server_options options);

        auto start() -> shared::result<bool>;
        void stop();

        auto tick(int timeout_ms) -> bool;

        void run();

        [[nodiscard]] auto port() const -> uint16_t { return options_.port; }
        [[nodiscard]] auto player_count() const -> size_t { return sessions_.size(); }
        [[nodiscard]] auto get_world() -> world & { return world_; }

      private:
        void on_accepted(const net::connection_ptr &link);
        void service(const std::shared_ptr<session> &player);
        void dispatch(const std::shared_ptr<session> &player, net::packet_reader &reader);

        void handle_login(const std::shared_ptr<session> &player, net::packet_reader &reader);
        void handle_create_account(const std::shared_ptr<session> &player, net::packet_reader &reader);

        [[nodiscard]]
        auto issue_token_for(std::string_view account) const -> std::string;

        void handle_move(const std::shared_ptr<session> &player, net::packet_reader &reader);
        void handle_chat(const std::shared_ptr<session> &player, net::packet_reader &reader);
        void handle_file_request(const std::shared_ptr<session> &player, net::packet_reader &reader);
        void handle_trigger(const std::shared_ptr<session> &player, net::packet_reader &reader);
        void handle_trigger_action(const std::shared_ptr<session> &player, net::packet_reader &reader);
        void handle_client_flag(const std::shared_ptr<session> &player, net::packet_reader &reader);
        void handle_edit_request(const std::shared_ptr<session> &player, net::packet_reader &reader);
        void handle_edit_release(const std::shared_ptr<session> &player, net::packet_reader &reader);
        void handle_edit_list_directory(const std::shared_ptr<session> &player, net::packet_reader &reader);
        void handle_edit_fetch_level(const std::shared_ptr<session> &player, net::packet_reader &reader);
        void handle_edit_save_level(const std::shared_ptr<session> &player, net::packet_reader &reader);
        void release_edit_locks(session &player);
        void reload_level(const std::string &level_name, std::shared_ptr<shared::level_data> level, const std::string &content);

        void send_npcs(const std::shared_ptr<session> &player) const;
        void send_items(const std::shared_ptr<session> &player) const;
        void flush_npcs();

        auto follow_links(const std::shared_ptr<session> &player) -> bool;
        void collect_items(const std::shared_ptr<session> &player);

        void send_flag(const std::shared_ptr<session> &player, net::flag_store store, const std::string &name, const std::string &text);

        void send_weapon(const std::shared_ptr<session> &player, const std::string &name) const;
        void flush_flags();
        void flush_warps();
        void flush_players();

        void enter_level(const std::shared_ptr<session> &player, const std::string &level_name, float x, float y);
        void leave_level(const std::shared_ptr<session> &player);
        void drop(const std::shared_ptr<session> &player, std::string_view reason);
        void persist(const session &player) const;

        void send_to_level(const std::string &level_name, const net::packet_writer &writer, const session *except) const;

        [[nodiscard]] auto find_by_account(std::string_view name) const -> std::shared_ptr<session>;
        [[nodiscard]] auto account_path(std::string_view name) const -> std::filesystem::path;
        [[nodiscard]] auto next_player_id() -> uint16_t;

        server_options options_;
        world world_;
        script_host scripts_;
        asio::io_context io_;
        asio::signal_set signals_;
        net::acceptor acceptor_;
        std::vector<std::shared_ptr<session>> sessions_;
        std::map<std::string, uint16_t> edit_locks_;
        std::vector<level_item> items_;
        std::vector<pending_warp> pending_warps_;
        std::string jwt_secret_;
        uint32_t next_item_id_ = 1;
        uint16_t next_id_ = 1;
        bool running_ = false;
        std::chrono::steady_clock::time_point last_tick_;
    };
}
