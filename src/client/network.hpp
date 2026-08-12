#pragma once

#include <net/connection.hpp>

#include <array>
#include <functional>
#include <map>
#include <string>
#include <vector>

class remote_player;

struct network_settings {
    std::string host = "127.0.0.1";
    uint16_t port = 14900;
    std::string account;
    std::string password;
    std::string nickname;
    std::string token;
};

class network {
  public:
    using level_handler = std::function<void(const std::string &level, float x, float y)>;
    using chat_handler = std::function<void(const std::string &nickname, const std::string &text)>;
    using weapon_handler = std::function<void(const std::string &name, const std::string &image, const std::string &source)>;
    using weapon_removed_handler = std::function<void(const std::string &name)>;
    using flag_handler = std::function<void(og::net::flag_store store, const std::string &name, const std::string &value)>;
    using action_handler = std::function<void(const std::string &name, const std::string &action, const std::vector<std::string> &args)>;

    struct gmap_terrain {
        int width = 0;
        int height = 0;
        float level_height = 0.0f;
        float level_chaos = 0.0f;
        uint32_t seed = 0;
        std::vector<int> heights;
    };

    using gmap_handler = std::function<void(const std::string &name, int cell_x, int cell_y, const std::array<std::string, 9> &neighbours, const gmap_terrain &terrain)>;
    using file_handler = std::function<void(const std::string &name, const std::string &data)>;
    using warp_handler = std::function<void()>;
    using edit_response_handler = std::function<void(const std::string &level, og::net::edit_status status, const std::string &detail)>;
    using edit_listing_handler = std::function<void(int category, const std::vector<std::string> &names)>;
    using edit_content_handler = std::function<void(const std::string &level, og::net::edit_status status, const std::string &data)>;
    using edit_save_handler = std::function<void(const std::string &level, og::net::edit_status status, const std::string &detail)>;
    using level_reload_handler = std::function<void(const std::string &level, const std::string &data)>;
    using login_result_handler = std::function<void(og::net::login_status status)>;
    using create_result_handler = std::function<void(og::net::create_status status, const std::string &message)>;
    using link_lost_handler = std::function<void()>;

    [[nodiscard]] auto is_enabled() const -> bool { return enabled_; }
    [[nodiscard]] auto is_connected() const -> bool { return connection_ && connection_->connected(); }

    [[nodiscard]] auto get_status() const -> const std::string & { return status_; }
    [[nodiscard]] auto get_players() const -> const std::map<uint16_t, remote_player> & { return players_; }
    [[nodiscard]] auto get_own_id() const -> uint16_t { return own_id_; }
    [[nodiscard]] auto get_account() const -> const std::string & { return settings_.account; }
    [[nodiscard]] auto get_host() const -> const std::string & { return settings_.host; }
    [[nodiscard]] auto get_port() const -> uint16_t { return settings_.port; }
    [[nodiscard]] auto get_token() const -> const std::string & { return settings_.token; }
    [[nodiscard]] auto get_npcs() const -> const std::map<uint32_t, og::net::npc_state> & { return npcs_; }
    [[nodiscard]] auto get_items() const -> const std::map<uint32_t, og::net::item_state> & { return items_; }

    auto connect(const network_settings &settings) -> bool;

    void send_login();
    void send_create_account(const std::string &account, const std::string &password);

    void warp_to(const std::string &host, uint16_t port, const std::string &token);
    void disconnect();

    void update(float dt);

    void send_move(float x, float y, int direction, const std::string &animation);
    void send_chat(const std::string &text);
    void send_trigger(const std::string &name, const std::string &action, const std::vector<std::string> &args);
    void send_client_flag(const std::string &name, const std::string &text);
    void send_action(float x, float y, const std::string &action, const std::vector<std::string> &args);
    void send_edit_request(const std::string &name);
    void send_edit_release(const std::string &name);
    void send_edit_list_directory(int category);
    void send_edit_fetch_level(const std::string &name);
    void send_edit_save_level(const std::string &name, const std::string &content);

    void on_level(level_handler handler) { level_handler_ = std::move(handler); }
    void on_chat(chat_handler handler) { chat_handler_ = std::move(handler); }
    void on_weapon(weapon_handler handler) { weapon_handler_ = std::move(handler); }
    void on_weapon_removed(weapon_removed_handler handler) { weapon_removed_handler_ = std::move(handler); }
    void on_flag(flag_handler handler) { flag_handler_ = std::move(handler); }
    void on_action(action_handler handler) { action_handler_ = std::move(handler); }
    void on_gmap(gmap_handler handler) { gmap_handler_ = std::move(handler); }
    void on_file(file_handler handler) { file_handler_ = std::move(handler); }
    void on_warp(warp_handler handler) { warp_handler_ = std::move(handler); }
    void on_edit_response(edit_response_handler handler) { edit_response_handler_ = std::move(handler); }
    void on_edit_listing(edit_listing_handler handler) { edit_listing_handler_ = std::move(handler); }
    void on_edit_content(edit_content_handler handler) { edit_content_handler_ = std::move(handler); }
    void on_edit_save(edit_save_handler handler) { edit_save_handler_ = std::move(handler); }
    void on_level_reload(level_reload_handler handler) { level_reload_handler_ = std::move(handler); }
    void on_login_result(login_result_handler handler) { login_result_handler_ = std::move(handler); }
    void on_create_result(create_result_handler handler) { create_result_handler_ = std::move(handler); }
    void on_link_lost(link_lost_handler handler) { link_lost_handler_ = std::move(handler); }

    void request_file(const std::string &name);

  private:
    void handle(og::net::packet_reader &reader);
    void retry(float dt);

    asio::io_context io_;
    og::net::connection_ptr connection_;
    std::map<uint16_t, remote_player> players_;
    std::map<uint32_t, og::net::npc_state> npcs_;
    std::map<uint32_t, og::net::item_state> items_;
    level_handler level_handler_;
    chat_handler chat_handler_;
    weapon_handler weapon_handler_;
    weapon_removed_handler weapon_removed_handler_;
    flag_handler flag_handler_;
    action_handler action_handler_;
    gmap_handler gmap_handler_;
    file_handler file_handler_;
    warp_handler warp_handler_;
    edit_response_handler edit_response_handler_;
    edit_listing_handler edit_listing_handler_;
    edit_content_handler edit_content_handler_;
    edit_save_handler edit_save_handler_;
    level_reload_handler level_reload_handler_;
    login_result_handler login_result_handler_;
    create_result_handler create_result_handler_;
    link_lost_handler link_lost_handler_;
    std::string status_ = "offline";
    uint16_t own_id_ = 0;
    bool enabled_ = false;
    bool reported_loss_ = false;
    bool refused_ = false;
    int attempts_ = 0;
    float retry_timer_ = 0.0f;
    network_settings settings_;

    float send_timer_ = 0.0f;
    float last_sent_x_ = 0.0f;
    float last_sent_y_ = 0.0f;
    int last_sent_direction_ = -1;
    std::string last_sent_animation_;
};

[[nodiscard]] auto get_network() -> network &;
