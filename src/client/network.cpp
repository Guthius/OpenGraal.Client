#include "network.hpp"

#include <array>
#include <format>
#include <ranges>

#include "constants.hpp"
#include "game.hpp"
#include "player.hpp"
#include "remote_player.hpp"

#include <raylib.h>

using namespace og::net;

namespace {
    constexpr float send_interval = 0.05f;
    constexpr float position_epsilon = 0.01f;
    constexpr int max_retries = 5;
    constexpr std::array retry_backoff{1.0f, 2.0f, 4.0f, 8.0f, 8.0f, 8.0f};

    network instance;
}

auto get_network() -> network & {
    return instance;
}

auto network::connect(const network_settings &settings) -> bool {
    connection_ = connection::connect(io_, settings.host, settings.port);

    if (!connection_) {
        status_ = "could not reach " + settings.host;

        TraceLog(LOG_ERROR, "NETWORK: %s", status_.c_str());

        return false;
    }

    enabled_ = true;
    reported_loss_ = false;
    refused_ = false;
    settings_ = settings;
    status_ = "connecting";

    TraceLog(LOG_INFO, "NETWORK: connected to %s:%d", settings.host.c_str(), settings.port);

    return true;
}

void network::send_login() {
    if (!connection_ || !connection_->connected()) {
        return;
    }

    auto login = packet_writer(message_id::login);
    login.put_u16(protocol_version)
        .put_string(settings_.account)
        .put_string(settings_.password)
        .put_string(settings_.nickname)
        .put_string(settings_.token);

    connection_->send(login);

    TraceLog(LOG_INFO, "NETWORK: logging in as %s", settings_.account.c_str());
}

void network::send_create_account(const std::string &account, const std::string &password) {
    if (!connection_ || !connection_->connected()) {
        return;
    }

    auto message = packet_writer(message_id::create_account);
    message.put_u16(protocol_version).put_string(account).put_string(password);

    connection_->send(message);
}

void network::disconnect() {
    if (connection_) {
        connection_->close();
    }

    io_.restart();
    io_.poll();

    players_.clear();
    enabled_ = false;
    status_ = "offline";
}

void network::update(const float dt) {
    if (!enabled_) {
        return;
    }

    io_.restart();
    io_.poll();

    while (connection_) {
        auto message = connection_->poll();
        if (!message) {
            break;
        }

        handle(*message);
    }

    if (!connection_ || !connection_->connected()) {
        if (!reported_loss_) {
            reported_loss_ = true;

            if (status_ == "connected" || status_ == "connecting") {
                status_ = "disconnected";
            }

            TraceLog(LOG_WARNING, "NETWORK: %s", status_.c_str());
        }

        players_.clear();
        npcs_.clear();

        retry(dt);

        return;
    }

    send_timer_ += dt;

    for (auto &[id, player] : players_) {
        player.update(dt);
    }
}

void network::send_client_flag(const std::string &name, const std::string &text) {
    if (!connection_ || !connection_->connected()) {
        return;
    }

    auto message = packet_writer(message_id::flag_changed);
    message.put_string(name).put_string(text);

    connection_->send(message);
}

void network::handle(packet_reader &reader) {
    switch (reader.id()) {
    case message_id::login_result:
        {
            const auto status = static_cast<login_status>(reader.get_u8());
            const auto token = reader.get_string();

            status_ = describe(status);

            if (status == login_status::ok) {
                settings_.token = token;
                attempts_ = 0;
            } else {
                refused_ = true;
                settings_.token.clear();
            }

            TraceLog(status == login_status::ok ? LOG_INFO : LOG_ERROR, "NETWORK: %s", status_.c_str());

            if (login_result_handler_) {
                login_result_handler_(status);
            }
        }
        break;

    case message_id::create_account_result:
        {
            const auto status = static_cast<create_status>(reader.get_u8());
            const auto message = reader.get_string();

            TraceLog(status == create_status::ok ? LOG_INFO : LOG_ERROR, "NETWORK: %s", message.c_str());

            if (create_result_handler_) {
                create_result_handler_(status, message);
            }
        }
        break;

    case message_id::player_identity:
        own_id_ = reader.get_u16();
        break;

    case message_id::level_enter:
        {
            const auto level = reader.get_string();
            const auto x = reader.get_float();
            const auto y = reader.get_float();

            if (level_handler_) {
                level_handler_(level, x, y);
            }
        }
        break;

    case message_id::player_add:
    case message_id::player_update:
        {
            const auto state = read_player_state(reader);

            if (state.id == own_id_) {
                if (auto *local = get_local_player()) {
                    local->set_appearance(state.head, state.body, state.attr);
                }

                break;
            }

            if (const auto match = players_.find(state.id); match != players_.end()) {
                match->second.apply(state);
            } else {
                players_.emplace(state.id, remote_player(state));
            }
        }
        break;

    case message_id::player_remove:
        players_.erase(reader.get_u16());
        break;

    case message_id::player_chat:
        {
            const auto id = reader.get_u16();
            const auto text = reader.get_string();

            const auto match = players_.find(id);
            const auto nickname = match == players_.end() ? std::string("you") : match->second.get_nickname();

            if (match != players_.end()) {
                match->second.say(text);
            }

            if (chat_handler_) {
                chat_handler_(nickname, text);
            }
        }
        break;

    case message_id::weapon_script:
        {
            const auto name = reader.get_string();
            const auto image = reader.get_string();
            const auto source = reader.get_string();

            if (weapon_handler_) {
                weapon_handler_(name, image, source);
            }
        }
        break;

    case message_id::weapon_remove:
        {
            const auto name = reader.get_string();

            if (weapon_removed_handler_) {
                weapon_removed_handler_(name);
            }
        }
        break;

    case message_id::flag_set:
        {
            const auto store = static_cast<og::net::flag_store>(reader.get_u8());
            const auto name = reader.get_string();
            const auto text = reader.get_string();

            if (flag_handler_) {
                flag_handler_(store, name, text);
            }
        }
        break;

    case message_id::trigger_client:
        {
            const auto name = reader.get_string();
            const auto action = reader.get_string();
            const auto count = reader.get_u16();

            std::vector<std::string> args;
            args.reserve(count);

            for (uint16_t i = 0; i < count; ++i) {
                args.push_back(reader.get_string());
            }

            if (action_handler_) {
                action_handler_(name, action, args);
            }
        }
        break;

    case message_id::file_data:
        {
            const auto name = reader.get_string();
            const auto data = reader.get_bytes();

            if (file_handler_) {
                file_handler_(name, std::string(data.begin(), data.end()));
            }
        }
        break;

    case message_id::file_missing:
        TraceLog(LOG_WARNING, "NETWORK: server has no file '%s'", reader.get_string().c_str());
        break;

    case message_id::edit_response:
        {
            const auto level = reader.get_string();
            const auto status = static_cast<edit_status>(reader.get_u8());
            const auto detail = reader.get_string();

            if (edit_response_handler_) {
                edit_response_handler_(level, status, detail);
            }
        }
        break;

    case message_id::edit_directory_listing:
        {
            const auto category = static_cast<int>(reader.get_u8());
            const auto count = reader.get_u32();

            std::vector<std::string> names;
            names.reserve(count);

            for (uint32_t i = 0; i < count && !reader.failed(); ++i) {
                names.push_back(reader.get_string());
            }

            if (edit_listing_handler_) {
                edit_listing_handler_(category, names);
            }
        }
        break;

    case message_id::edit_level_content:
        {
            const auto level = reader.get_string();
            const auto status = static_cast<edit_status>(reader.get_u8());
            const auto data = reader.get_bytes();

            if (edit_content_handler_) {
                edit_content_handler_(level, status, std::string(data.begin(), data.end()));
            }
        }
        break;

    case message_id::edit_save_result:
        {
            const auto level = reader.get_string();
            const auto status = static_cast<edit_status>(reader.get_u8());
            const auto detail = reader.get_string();

            if (edit_save_handler_) {
                edit_save_handler_(level, status, detail);
            }
        }
        break;

    case message_id::level_reload:
        {
            const auto level = reader.get_string();
            const auto data = reader.get_bytes();

            if (level_reload_handler_) {
                level_reload_handler_(level, std::string(data.begin(), data.end()));
            }
        }
        break;

    case message_id::level_gmap:
        {
            const auto name = reader.get_string();
            const auto width = reader.get_u16();
            const auto height = reader.get_u16();
            const auto cell_x = reader.get_u16();
            const auto cell_y = reader.get_u16();

            std::array<std::string, 9> neighbours;
            for (auto &neighbour : neighbours) {
                neighbour = reader.get_string();
            }

            network::gmap_terrain terrain;
            terrain.width = width;
            terrain.height = height;
            terrain.level_height = reader.get_float();
            terrain.level_chaos = reader.get_float();
            terrain.seed = reader.get_u32();

            for (const auto raw = reader.get_bytes(); const auto index : std::views::iota(size_t{0}, raw.size() / 2)) {
                terrain.heights.push_back(static_cast<int16_t>(raw[index * 2] | (raw[(index * 2) + 1] << 8)));
            }

            if (gmap_handler_ && width > 0 && height > 0) {
                gmap_handler_(name, cell_x, cell_y, neighbours, terrain);
            }
        }
        break;

    case message_id::item_add:
        {
            const auto state = read_item_state(reader);
            items_[state.id] = state;
        }
        break;

    case message_id::item_remove:
        items_.erase(reader.get_u32());
        break;

    case message_id::npc_add:
    case message_id::npc_update:
        {
            const auto state = read_npc_state(reader);
            npcs_[state.id] = state;
        }
        break;

    case message_id::npc_remove:
        npcs_.erase(reader.get_u32());
        break;

    case message_id::server_warp:
        {
            const auto host = reader.get_string();
            const auto port = reader.get_u16();
            const auto token = reader.get_string();

            warp_to(host, port, token);
        }
        break;

    case message_id::disconnect:
        status_ = "disconnected: " + reader.get_string();
        connection_->close();
        break;

    default:
        break;
    }
}

void network::send_move(const float x, const float y, const int direction, const std::string &animation) {
    if (!is_connected()) {
        return;
    }

    const auto tile_x = x / tile_size;
    const auto tile_y = y / tile_size;

    const auto unchanged =
        std::abs(tile_x - last_sent_x_) < position_epsilon &&
        std::abs(tile_y - last_sent_y_) < position_epsilon &&
        direction == last_sent_direction_ &&
        animation == last_sent_animation_;

    if (unchanged || send_timer_ < send_interval) {
        return;
    }

    send_timer_ = 0.0f;
    last_sent_x_ = tile_x;
    last_sent_y_ = tile_y;
    last_sent_direction_ = direction;
    last_sent_animation_ = animation;

    auto message = packet_writer(message_id::move);
    message.put_float(tile_x).put_float(tile_y).put_u8(static_cast<uint8_t>(direction)).put_string(animation);
    connection_->send(message);
}

void network::send_trigger(const std::string &name, const std::string &action, const std::vector<std::string> &args) {
    if (!is_connected()) {
        return;
    }

    auto message = packet_writer(message_id::trigger_server);
    message.put_string(name).put_string(action).put_u16(static_cast<uint16_t>(args.size()));

    for (const auto &argument : args) {
        message.put_string(argument);
    }

    connection_->send(message);
}

void network::send_action(const float x, const float y, const std::string &action, const std::vector<std::string> &args) {
    if (!is_connected()) {
        return;
    }

    auto message = packet_writer(message_id::trigger_action);
    message.put_float(x).put_float(y).put_string(action).put_u16(static_cast<uint16_t>(args.size()));

    for (const auto &argument : args) {
        message.put_string(argument);
    }

    connection_->send(message);
}

void network::warp_to(const std::string &host, const uint16_t port, const std::string &token) {
    auto target = settings_;

    if (!host.empty()) {
        target.host = host;
    }

    if (port != 0) {
        target.port = port;
    }

    target.token = token;

    const auto previous = settings_;

    disconnect();

    if (warp_handler_) {
        warp_handler_();
    }

    if (connect(target)) {
        send_login();

        return;
    }

    TraceLog(LOG_ERROR, "NETWORK: warp to %s:%d failed, returning to %s:%d",
        target.host.c_str(), target.port, previous.host.c_str(), previous.port);

    if (connect(previous)) {
        send_login();
    } else if (link_lost_handler_) {
        link_lost_handler_();
    }
}

void network::retry(const float dt) {
    if (refused_ || settings_.account.empty()) {
        return;
    }

    if (attempts_ >= max_retries) {
        if (link_lost_handler_) {
            attempts_ = 0;
            enabled_ = false;
            link_lost_handler_();
        }

        return;
    }

    retry_timer_ -= dt;

    if (retry_timer_ > 0.0f) {
        return;
    }

    ++attempts_;

    status_ = std::format("reconnecting ({}/{})", attempts_, max_retries);
    TraceLog(LOG_INFO, "NETWORK: %s", status_.c_str());

    if (connect(settings_)) {
        send_login();

        return;
    }

    retry_timer_ = retry_backoff[std::min<size_t>(attempts_, retry_backoff.size() - 1)];
}

void network::request_file(const std::string &name) {
    if (!is_connected() || name.empty()) {
        return;
    }

    connection_->send(packet_writer(message_id::request_file).put_string(name));
}

void network::send_chat(const std::string &text) {
    if (!is_connected() || text.empty()) {
        return;
    }

    connection_->send(packet_writer(message_id::chat).put_string(text));
}

void network::send_edit_request(const std::string &name) {
    if (!is_connected()) {
        return;
    }

    connection_->send(packet_writer(message_id::edit_request).put_string(name));
}

void network::send_edit_release(const std::string &name) {
    if (!is_connected()) {
        return;
    }

    connection_->send(packet_writer(message_id::edit_release).put_string(name));
}

void network::send_edit_list_directory(const int category) {
    if (!is_connected()) {
        return;
    }

    connection_->send(packet_writer(message_id::edit_list_directory).put_u8(static_cast<uint8_t>(category)));
}

void network::send_edit_fetch_level(const std::string &name) {
    if (!is_connected() || name.empty()) {
        return;
    }

    connection_->send(packet_writer(message_id::edit_fetch_level).put_string(name));
}

void network::send_edit_save_level(const std::string &name, const std::string &content) {
    if (!is_connected() || name.empty()) {
        return;
    }

    connection_->send(packet_writer(message_id::edit_save_level)
            .put_string(name)
            .put_bytes(std::vector<uint8_t>(content.begin(), content.end())));
}
