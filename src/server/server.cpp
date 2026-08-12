#include <server/server.hpp>

#include <shared/container.hpp>
#include <shared/random.hpp>
#include <shared/text.hpp>
#include <shared/token.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <sstream>
#include <system_error>

using namespace std;
using namespace og::net;
using namespace og::gs2;
using namespace og::shared;

namespace og::server {
    namespace {
        auto link_contains(const shared::level_link &link, const float x, const float y) -> bool {
            return x >= static_cast<float>(link.x) && x < static_cast<float>(link.x + link.width) &&
                   y >= static_cast<float>(link.y) && y < static_cast<float>(link.y + link.height);
        }

        auto touch_point(const net::player_state &state) -> pair<float, float> {
            constexpr float reach = 17.0f / 16.0f;

            switch (state.direction % 4) {
            case 0:  return {state.x + 1.0f, state.y + 1.0f - reach};
            case 1:  return {state.x + 1.0f - reach, state.y + 1.0f};
            case 2:  return {state.x + 1.0f, state.y + 1.0f + reach};
            default: return {state.x + 1.0f + reach, state.y + 1.0f};
            }
        }

        auto on_any_link(const shared::level_data &level, const net::player_state &state) -> bool {
            const auto [touch_x, touch_y] = touch_point(state);

            return ranges::any_of(level.links, [&](const shared::level_link &link) {
                return link_contains(link, state.x, state.y) || link_contains(link, touch_x, touch_y);
            });
        }

        constexpr float max_move_per_update = 8.0f;
        constexpr size_t max_chat_length = 220;
        constexpr size_t max_name_length = 32;
        constexpr size_t max_account_length = 254;
        constexpr size_t min_password_length = 6;
        constexpr size_t min_secret_length = 32;
        constexpr size_t max_create_attempts = 5;
        constexpr int64_t token_lifetime = 120;

        auto sanitise(string text, const size_t limit) -> string {
            erase_if(text, [](const unsigned char ch) { return ch < 0x20 && ch != '\t'; });

            if (text.size() > limit) {
                text.resize(limit);
            }

            return text;
        }

        auto default_nickname(const string_view account) -> string {
            return string(account.substr(0, account.find('@')));
        }

        auto secret_path(const filesystem::path &world) -> filesystem::path {
            return world / "jwt_secret.txt";
        }

        auto read_secret(const filesystem::path &path) -> string {
            ifstream ifs(path);
            string line;

            return getline(ifs, line) ? trim(line) : string{};
        }

        auto write_secret(const filesystem::path &path, const string_view secret) -> bool {
            ofstream ofs(path, ios::trunc);

            return ofs && (ofs << secret << '\n').good();
        }
    }

    auto session::identifier() const -> string {
        return profile.name.empty() ? format("#{}", state.id) : profile.name;
    }

    server::server(server_options options)
        : options_(std::move(options)),
          scripts_(world_),
          signals_(io_, SIGINT, SIGTERM),
          acceptor_(io_, [this](const connection_ptr &link) { on_accepted(link); }) {

        signals_.async_wait([this](const asio::error_code &code, int) {
            if (!code) {
                println("\nShutting down");
                running_ = false;
            }
        });

        scripts_.on_chat([this](const npc &source, const string &text) {
            auto message = packet_writer(message_id::npc_update);
            write_npc_state(message, source.state);
            send_to_level(source.level, message, nullptr);

            println("[{}] {}: {}", source.level, source.state.nickname.empty() ? source.name : source.state.nickname, text);
        });

        scripts_.on_put_item([this](const string &level, const float x, const float y, const string &name, const string &image) {
            auto item = level_item{
                .state = net::item_state{.id = next_item_id_++, .x = x, .y = y, .name = name, .image = image},
                .level = level,
            };

            auto message = packet_writer(message_id::item_add);
            write_item_state(message, item.state);
            send_to_level(level, message, nullptr);

            items_.push_back(std::move(item));
        });

        scripts_.on_server_warp([this](const connection_ptr &target, const string &host, const uint16_t port, const string &account) {
            if (!target) {
                return;
            }

            auto message = packet_writer(message_id::server_warp);
            message.put_string(host).put_u16(port).put_string(issue_token_for(account));
            target->send(message);
        });

        scripts_.on_trigger_client([](const connection_ptr &target, const string &name, const string &action, const values &args) {
            if (!target) {
                return;
            }

            auto message = packet_writer(message_id::trigger_client);
            message.put_string(name).put_string(action).put_u16(static_cast<uint16_t>(args.size()));

            for (const auto &argument : args) {
                message.put_string(gs2::to_string(argument));
            }

            target->send(message);
        });

        scripts_.on_set_level([this](session &player, const string &level, const float x, const float y) {
            if (const auto target = find_by_account(player.profile.name)) {
                pending_warps_.push_back({.player = target, .level = level, .x = x, .y = y});
            }
        });

        scripts_.on_list_players([this] {
            vector<session *> online;
            online.reserve(sessions_.size());

            for (const auto &player : sessions_) {
                if (player->authenticated) {
                    online.push_back(player.get());
                }
            }

            return online;
        });

        scripts_.on_weapon_changed([this](session &player, const string &name, const bool granted) {
            const auto owner = find_by_account(player.profile.name);
            if (!owner) {
                return;
            }

            if (granted) {
                flush_flags();
                send_weapon(owner, name);
            } else if (owner->connection) {
                owner->connection->send(packet_writer(message_id::weapon_remove).put_string(name));
            }
        });
    }

    auto server::start() -> result<bool> {
        if (auto loaded = world_.load(options_.world_path); !loaded) {
            return make_error(loaded.error());
        }

        if (!acceptor_.listen(options_.port)) {
            return make_error(format("could not listen on port {}", options_.port));
        }

        options_.port = acceptor_.port();

        jwt_secret_ = world_.options().get("jwt_secret");

        if (jwt_secret_.empty()) {
            jwt_secret_ = read_secret(secret_path(options_.world_path));
        }

        if (!jwt_secret_.empty() && jwt_secret_.size() < min_secret_length) {
            println("jwt_secret is shorter than {} characters and will be ignored", min_secret_length);
            jwt_secret_.clear();
        }

        if (jwt_secret_.empty()) {
            const auto bytes = shared::random_bytes(32);
            jwt_secret_ = shared::base64url_encode({reinterpret_cast<const char *>(bytes.data()), bytes.size()});

            if (write_secret(secret_path(options_.world_path), jwt_secret_)) {
                println("generated {}; copy it to any serverwarp target so it accepts our tokens", secret_path(options_.world_path).string());
            } else {
                println("could not persist a jwt_secret; serverwarp will re-prompt after a restart");
            }
        }

        scripts_.load();
        scripts_.ensure_level(world_.start().level);

        last_tick_ = chrono::steady_clock::now();

        running_ = true;

        return true;
    }

    void server::stop() {
        for (const auto &player : sessions_) {
            persist(*player);
            player->connection->close();
        }

        sessions_.clear();
        acceptor_.close();
        io_.poll();
        running_ = false;
    }

    void server::run() {
        while (tick(50)) {
        }

        stop();
    }

    auto server::tick(const int timeout_ms) -> bool {
        if (!running_) {
            return false;
        }

        io_.restart();
        io_.run_for(chrono::milliseconds(timeout_ms));

        const auto now = chrono::steady_clock::now();
        const auto elapsed = chrono::duration<double>(now - last_tick_).count();
        last_tick_ = now;

        scripts_.tick(elapsed);
        flush_npcs();
        flush_flags();
        flush_players();
        flush_warps();

        const auto snapshot = sessions_;
        for (const auto &player : snapshot) {
            service(player);
        }

        erase_if(sessions_, [](const shared_ptr<session> &player) {
            return !player->connection->connected();
        });

        return running_;
    }

    void server::on_accepted(const connection_ptr &link) {
        auto player = make_shared<session>();
        player->connection = link;
        player->state.id = next_player_id();

        if (sessions_.size() >= options_.max_players) {
            link->send(packet_writer(message_id::login_result).put_u8(static_cast<uint8_t>(login_status::server_full)));
            link->close();

            return;
        }

        sessions_.push_back(player);
    }

    void server::service(const shared_ptr<session> &player) {
        while (auto message = player->connection->poll()) {
            dispatch(player, *message);

            if (!player->connection->connected()) {
                break;
            }
        }

        if (!player->connection->connected() && player->authenticated) {
            scripts_.player_offline(*player);

            persist(*player);
            leave_level(player);
            release_edit_locks(*player);

            player->authenticated = false;

            println("[-] {} disconnected", player->identifier());
        }
    }

    void server::dispatch(const shared_ptr<session> &player, packet_reader &reader) {
        if (!player->authenticated && reader.id() != message_id::login && reader.id() != message_id::create_account) {
            drop(player, "not authenticated");

            return;
        }

        switch (reader.id()) {
        case message_id::login:          handle_login(player, reader); break;
        case message_id::create_account: handle_create_account(player, reader); break;
        case message_id::move:           handle_move(player, reader); break;
        case message_id::chat:           handle_chat(player, reader); break;
        case message_id::request_file:   handle_file_request(player, reader); break;
        case message_id::trigger_server: handle_trigger(player, reader); break;
        case message_id::trigger_action: handle_trigger_action(player, reader); break;
        case message_id::flag_changed:   handle_client_flag(player, reader); break;

        case message_id::edit_request:        handle_edit_request(player, reader); break;
        case message_id::edit_release:        handle_edit_release(player, reader); break;
        case message_id::edit_list_directory: handle_edit_list_directory(player, reader); break;
        case message_id::edit_fetch_level:    handle_edit_fetch_level(player, reader); break;
        case message_id::edit_save_level:     handle_edit_save_level(player, reader); break;

        case message_id::ping:
            player->connection->send(packet_writer(message_id::pong).put_u32(reader.get_u32()));
            break;

        default:
            break;
        }

        if (reader.failed()) {
            drop(player, "malformed message");
        }
    }

    auto server::issue_token_for(const string_view account) const -> string {
        if (jwt_secret_.empty()) {
            return {};
        }

        const auto now = shared::unix_now();

        return shared::issue_token({.account = string(account), .issued_at = now, .expires_at = now + token_lifetime}, jwt_secret_);
    }

    void server::handle_create_account(const shared_ptr<session> &player, packet_reader &reader) {
        const auto version = reader.get_u16();
        const auto name = to_lower(sanitise(reader.get_string(), max_account_length));
        const auto password = reader.get_string();

        const auto reply = [&player](const create_status status) {
            player->connection->send(packet_writer(message_id::create_account_result)
                    .put_u8(static_cast<uint8_t>(status))
                    .put_string(string(describe(status))));
        };

        if (++player->creates_attempted > max_create_attempts) {
            drop(player, "too many account attempts");

            return;
        }

        if (reader.failed() || version != protocol_version) {
            reply(create_status::bad_version);

            return;
        }

        if (!options_.allow_registration) {
            reply(create_status::not_allowed);

            return;
        }

        if (!is_valid_email(name)) {
            reply(create_status::bad_email);

            return;
        }

        if (password.size() < min_password_length) {
            reply(create_status::weak_password);

            return;
        }

        const auto path = account_path(name);

        if (exists(path)) {
            reply(create_status::already_exists);

            return;
        }

        const auto created = account{
            .name = name,
            .password_hash = hash_password(password),
            .nickname = default_nickname(name),
            .level = world_.start().level,
            .x = world_.start().x,
            .y = world_.start().y,
        };

        if (const auto saved = save_account(path, created); !saved) {
            println("could not create account '{}': {}", name, saved.error());

            reply(create_status::not_allowed);

            return;
        }

        println("[*] account created for {}", name);

        reply(create_status::ok);
    }

    void server::handle_login(const shared_ptr<session> &player, packet_reader &reader) {
        const auto version = reader.get_u16();
        const auto name = to_lower(sanitise(reader.get_string(), max_account_length));
        const auto password = reader.get_string();
        const auto nickname = sanitise(reader.get_string(), max_name_length);
        const auto token = reader.get_string();

        const auto refuse = [&player](const login_status status) {
            player->connection->send(packet_writer(message_id::login_result)
                    .put_u8(static_cast<uint8_t>(status))
                    .put_string(""));
            player->connection->close();
        };

        if (reader.failed() || version != protocol_version) {
            refuse(login_status::bad_version);

            return;
        }

        if (!is_valid_email(name)) {
            refuse(login_status::bad_credentials);

            return;
        }

        auto by_token = false;
        if (!token.empty()) {
            const auto claims = shared::verify_token(token, jwt_secret_, shared::unix_now());
            if (!claims || claims->account != name) {
                refuse(login_status::bad_credentials);

                return;
            }

            by_token = true;
        }

        if (const auto other = find_by_account(name)) {
            // A warp back to the same world beats its own teardown here; only a live one is a clash.
            if (other->connection && other->connection->connected()) {
                refuse(login_status::already_online);

                return;
            }

            service(other);
        }

        const auto path = account_path(name);

        if (auto stored = load_account(path)) {
            if (stored->banned) {
                refuse(login_status::bad_credentials);

                return;
            }

            if (!by_token) {
                switch (stored->verify(password)) {
                case verify_result::failed:
                    refuse(login_status::bad_credentials);

                    return;

                case verify_result::needs_rehash:
                    stored->password_hash = hash_password(password);
                    (void)save_account(path, *stored);
                    break;

                case verify_result::ok:
                    break;
                }
            }

            player->profile = std::move(*stored);
        } else if (options_.allow_registration) {
            player->profile = account{
                .name = string(name),
                .password_hash = by_token ? string{} : hash_password(password),
                .level = world_.start().level,
                .x = world_.start().x,
                .y = world_.start().y,
            };
        } else {
            refuse(login_status::bad_credentials);

            return;
        }

        if (player->profile.nickname.empty()) {
            player->profile.nickname = nickname.empty() ? default_nickname(name) : nickname;
        }

        player->authenticated = true;
        player->state.nickname = player->profile.nickname;
        player->state.head = player->profile.head;
        player->state.body = player->profile.body;
        player->state.direction = player->profile.direction;

        player->connection->send(packet_writer(message_id::login_result)
                .put_u8(static_cast<uint8_t>(login_status::ok))
                .put_string(issue_token_for(player->profile.name)));
        player->connection->send(packet_writer(message_id::player_identity).put_u16(player->state.id));

        const auto &level = player->profile.level.empty() ? world_.start().level : player->profile.level;

        for (const auto &name : scripts_.get_server_flags()->keys()) {
            send_flag(player, net::flag_store::serverr, name, to_string(scripts_.get_server_flags()->get(name)));
        }

        (void)scripts_.take_changed_server_flags();

        player->weapons = player->profile.weapons;

        for (const auto &name : player->weapons) {
            send_weapon(player, name);
        }

        scripts_.player_online(*player);
        enter_level(player, level, player->profile.x, player->profile.y);
        scripts_.player_logged_in(*player);

        println("[+] {} logged in at {} ({:.1f}, {:.1f})",
            player->identifier(), player->level, player->state.x, player->state.y);
    }

    void server::handle_client_flag(const shared_ptr<session> &player, packet_reader &reader) {
        const auto name = sanitise(reader.get_string(), max_name_length);
        const auto text = sanitise(reader.get_string(), max_chat_length);

        if (!player->authenticated || name.empty()) {
            return;
        }

        player->client_flags->put(name, gs2::value{text});
        (void)player->client_flags->take_changed();
    }

    void server::send_flag(const shared_ptr<session> &player, const net::flag_store store, const string &name, const string &text) {
        if (!player->connection) {
            return;
        }

        auto message = packet_writer(message_id::flag_set);

        message.put_u8(static_cast<uint8_t>(store)).put_string(name).put_string(text);

        player->connection->send(message);
    }

    void server::flush_warps() {
        for (const auto &warp : std::exchange(pending_warps_, {})) {
            if (warp.player->authenticated) {
                leave_level(warp.player);
                enter_level(warp.player, warp.level, warp.x, warp.y);
            }
        }
    }

    void server::send_weapon(const shared_ptr<session> &player, const string &name) const {
        const auto *entry = scripts_.find_weapon(name);
        if (!player->connection || entry == nullptr || entry->client_script.empty()) {
            return;
        }

        auto message = packet_writer(message_id::weapon_script);
        message.put_string(entry->name).put_string(entry->image).put_string(entry->client_script);

        player->connection->send(message);
    }

    void server::flush_players() {
        for (const auto &player : sessions_) {
            if (!player->state_changed) {
                continue;
            }

            player->state_changed = false;

            if (!player->authenticated) {
                continue;
            }

            auto update = packet_writer(message_id::player_update);
            write_player_state(update, player->state);

            player->connection->send(update);

            if (player->state.visible) {
                send_to_level(player->level, update, player.get());
            }
        }
    }

    void server::flush_flags() {
        for (const auto &name : scripts_.take_changed_server_flags()) {
            const auto text = to_string(scripts_.get_server_flags()->get(name));

            for (const auto &player : sessions_) {
                if (player->authenticated) {
                    send_flag(player, net::flag_store::serverr, name, text);
                }
            }
        }

        for (const auto &player : sessions_) {
            for (const auto &name : player->clientr_flags->take_changed()) {
                send_flag(player, net::flag_store::clientr, name, to_string(player->clientr_flags->get(name)));
            }
        }
    }

    void server::enter_level(const shared_ptr<session> &player, const string &level_name, float x, float y) {
        auto level = world_.find_level(level_name);

        if (!level) {
            for (const auto &map : world_.get_gmaps()) {
                if (!iequals(map.name, level_name)) {
                    continue;
                }

                const auto cell_x = static_cast<int>(x) / shared::level_width;
                const auto cell_y = static_cast<int>(y) / shared::level_height;

                if (auto found = world_.find_level(map.level_at(cell_x, cell_y))) {
                    level = std::move(found);
                    x -= static_cast<float>(cell_x * shared::level_width);
                    y -= static_cast<float>(cell_y * shared::level_height);
                }

                break;
            }
        }

        if (!level) {
            level = world_.find_level(world_.start().level);
        }

        if (!level) {
            drop(player, "no level available");

            return;
        }

        leave_level(player);

        player->level = level->name;
        player->state.x = x;
        player->state.y = y;
        player->link_grace = on_any_link(*level, player->state);

        auto enter = packet_writer(message_id::level_enter);
        enter.put_string(level->name).put_float(x).put_float(y);
        player->connection->send(enter);

        if (const auto [map, cell] = world_.find_gmap(level->name); map != nullptr) {
            auto info = packet_writer(message_id::level_gmap);
            info.put_string(map->name)
                .put_u16(static_cast<uint16_t>(map->width))
                .put_u16(static_cast<uint16_t>(map->height))
                .put_u16(static_cast<uint16_t>(cell.x))
                .put_u16(static_cast<uint16_t>(cell.y));

            for (int offset_y = -1; offset_y <= 1; ++offset_y) {
                for (int offset_x = -1; offset_x <= 1; ++offset_x) {
                    info.put_string(map->level_at(cell.x + offset_x, cell.y + offset_y));
                }
            }

            info.put_float(static_cast<float>(map->level_height));
            info.put_float(static_cast<float>(map->level_chaos));
            info.put_u32(map->random_seeds.empty() ? 0 : map->random_seeds.front());

            vector<uint8_t> heights;
            heights.reserve(map->height_map.size() * sizeof(int16_t));

            for (const auto height : map->height_map) {
                const auto clamped = static_cast<int16_t>(std::clamp(height, -32768, 32767));

                heights.push_back(static_cast<uint8_t>(clamped & 0xFF));
                heights.push_back(static_cast<uint8_t>((clamped >> 8) & 0xFF));
            }

            info.put_bytes(heights);

            player->connection->send(info);
        }

        if (const auto *tiles = level->tiles()) {
            vector<uint8_t> board;
            board.reserve(tiles->size() * sizeof(uint16_t));

            for (const auto tile : *tiles) {
                board.push_back(static_cast<uint8_t>(tile & 0xFF));
                board.push_back(static_cast<uint8_t>((tile >> 8) & 0xFF));
            }

            player->connection->send(packet_writer(message_id::level_board).put_bytes(board));
        }

        for (const auto &other : sessions_) {
            if (other == player || other->level != player->level || !other->authenticated) {
                continue;
            }

            if (!other->state.visible) {
                continue;
            }

            auto existing = packet_writer(message_id::player_add);
            write_player_state(existing, other->state);
            player->connection->send(existing);
        }

        if (player->state.visible) {
            auto arrival = packet_writer(message_id::player_add);
            write_player_state(arrival, player->state);
            send_to_level(player->level, arrival, player.get());
        }

        scripts_.ensure_level(player->level);
        send_npcs(player);
        send_items(player);
        scripts_.player_entered(*player);
    }

    void server::send_items(const shared_ptr<session> &player) const {
        for (const auto &item : items_) {
            if (item.level != player->level) {
                continue;
            }

            auto message = packet_writer(message_id::item_add);
            write_item_state(message, item.state);
            player->connection->send(message);
        }
    }

    auto server::follow_links(const shared_ptr<session> &player) -> bool {
        const auto level = world_.find_level(player->level);
        if (!level) {
            return false;
        }

        const auto [touch_x, touch_y] = touch_point(player->state);
        auto on_link = false;

        for (const auto &link : level->links) {
            const auto inside = link_contains(link, player->state.x, player->state.y) ||
                                link_contains(link, touch_x, touch_y);

            if (!inside) {
                continue;
            }

            on_link = true;

            if (player->link_grace) {
                break;
            }

            const auto resolve = [](const string &target, const float current) {
                return iequals(target, "playerx") || iequals(target, "playery")
                           ? current
                           : static_cast<float>(shared::to_float(target, current));
            };

            const auto destination_x = resolve(link.destination_x, player->state.x);
            const auto destination_y = resolve(link.destination_y, player->state.y);

            if (!world_.find_level(link.destination)) {
                continue;
            }

            enter_level(player, link.destination, destination_x, destination_y);

            auto warp = packet_writer(message_id::player_update);
            write_player_state(warp, player->state);
            player->connection->send(warp);

            return true;
        }

        if (!on_link) {
            player->link_grace = false;
        }

        return false;
    }

    void server::collect_items(const shared_ptr<session> &player) {
        constexpr float pickup_radius = 1.5f;

        erase_if(items_, [this, &player](const level_item &item) {
            if (item.level != player->level) {
                return false;
            }

            if (fabs(item.state.x - player->state.x) > pickup_radius ||
                fabs(item.state.y - player->state.y) > pickup_radius) {
                return false;
            }

            auto message = packet_writer(message_id::item_remove);
            message.put_u32(item.state.id);
            send_to_level(item.level, message, nullptr);

            scripts_.item_taken(*player, item.state.name);

            return true;
        });
    }

    void server::send_npcs(const shared_ptr<session> &player) const {
        for (const auto &entry : scripts_.npcs_in(player->level)) {
            auto message = packet_writer(message_id::npc_add);
            write_npc_state(message, entry->state);
            player->connection->send(message);
        }
    }

    void server::flush_npcs() {
        for (const auto &entry : scripts_.get_npcs()) {
            if (!entry->dirty) {
                continue;
            }

            entry->dirty = false;

            auto message = packet_writer(message_id::npc_update);
            write_npc_state(message, entry->state);
            send_to_level(entry->level, message, nullptr);
        }
    }

    void server::handle_trigger(const shared_ptr<session> &player, packet_reader &reader) {
        const auto name = reader.get_string();
        const auto action = reader.get_string();
        const auto count = reader.get_u16();

        values arguments;
        arguments.reserve(count);

        for (uint16_t i = 0; i < count && !reader.failed(); ++i) {
            arguments.emplace_back(reader.get_string());
        }

        if (reader.failed()) {
            drop(player, "malformed trigger");

            return;
        }

        scripts_.trigger_server(*player, name, action, arguments);
    }

    void server::handle_trigger_action(const shared_ptr<session> &player, packet_reader &reader) {
        const auto x = reader.get_float();
        const auto y = reader.get_float();
        const auto action = reader.get_string();
        const auto count = reader.get_u16();

        values arguments;
        arguments.reserve(count);

        for (uint16_t i = 0; i < count && !reader.failed(); ++i) {
            arguments.emplace_back(reader.get_string());
        }

        if (reader.failed()) {
            drop(player, "malformed action");

            return;
        }

        scripts_.trigger_action(*player, player->level, x, y, action, arguments);
    }

    void server::leave_level(const shared_ptr<session> &player) {
        if (player->level.empty()) {
            return;
        }

        scripts_.player_left(*player);

        send_to_level(player->level, packet_writer(message_id::player_remove).put_u16(player->state.id), player.get());
        player->level.clear();
    }

    void server::handle_move(const shared_ptr<session> &player, packet_reader &reader) {
        const auto x = reader.get_float();
        const auto y = reader.get_float();
        const auto direction = reader.get_u8();
        const auto animation = sanitise(reader.get_string(), 64);

        if (reader.failed() || !isfinite(x) || !isfinite(y)) {
            drop(player, "malformed move");

            return;
        }

        if (player->editing) {
            return;
        }

        const auto delta_x = fabs(x - player->state.x);
        const auto delta_y = fabs(y - player->state.y);

        if (delta_x > max_move_per_update || delta_y > max_move_per_update) {
            auto correction = packet_writer(message_id::player_update);
            write_player_state(correction, player->state);
            player->connection->send(correction);

            return;
        }

        player->state.x = clamp(x, 0.0f, static_cast<float>(shared::level_width));
        player->state.y = clamp(y, 0.0f, static_cast<float>(shared::level_height));
        player->state.direction = direction % 4;
        player->state.animation = animation.empty() ? "idle" : animation;

        if (follow_links(player)) {
            return;
        }

        collect_items(player);

        if (player->state.visible) {
            auto update = packet_writer(message_id::player_update);
            write_player_state(update, player->state);
            send_to_level(player->level, update, player.get());
        }
    }

    void server::handle_chat(const shared_ptr<session> &player, packet_reader &reader) {
        const auto text = sanitise(reader.get_string(), max_chat_length);
        if (reader.failed() || text.empty()) {
            return;
        }

        auto chat = packet_writer(message_id::player_chat);
        chat.put_u16(player->state.id).put_string(text);

        send_to_level(player->level, chat, nullptr);

        player->last_chat = text;
        scripts_.player_chatted(*player, text);

        println("[{}] {}: {}", player->level, player->identifier(), text);
    }

    void server::handle_file_request(const shared_ptr<session> &player, packet_reader &reader) {
        const auto name = reader.get_string();
        if (reader.failed()) {
            return;
        }

        const auto path = world_.find_asset(name);
        if (path.empty()) {
            player->connection->send(packet_writer(message_id::file_missing).put_string(name));

            return;
        }

        ifstream ifs(path, ios::binary);
        if (!ifs) {
            player->connection->send(packet_writer(message_id::file_missing).put_string(name));

            return;
        }

        vector<uint8_t> data((istreambuf_iterator(ifs)), istreambuf_iterator<char>());

        auto response = packet_writer(message_id::file_data);
        response.put_string(name).put_bytes(data);
        player->connection->send(response);
    }

    void server::handle_edit_request(const shared_ptr<session> &player, packet_reader &reader) {
        auto name = reader.get_string();
        if (reader.failed()) {
            return;
        }

        if (name.empty()) {
            name = player->level;
        }

        const auto respond = [&](const edit_status status, const string_view detail = {}) {
            player->connection->send(packet_writer(message_id::edit_response)
                    .put_string(name)
                    .put_u8(static_cast<uint8_t>(status))
                    .put_string(detail));
        };

        if (!player->profile.developer) {
            respond(edit_status::not_developer);

            return;
        }

        if (!world_.matches_category(shared::folder_category::level, name)) {
            respond(edit_status::bad_name);

            return;
        }

        if (world_.find_asset(name).empty()) {
            respond(edit_status::not_found);

            return;
        }

        const auto key = to_lower(name);

        if (const auto held = edit_locks_.find(key); held != edit_locks_.end() && held->second != player->state.id) {
            for (const auto &other : sessions_) {
                if (other->authenticated && other->state.id == held->second) {
                    respond(edit_status::locked, other->profile.name);

                    return;
                }
            }
        }

        edit_locks_[key] = player->state.id;
        player->edit_levels.insert(key);
        player->editing = true;

        respond(edit_status::ok);
    }

    void server::handle_edit_release(const shared_ptr<session> &player, packet_reader &reader) {
        const auto name = reader.get_string();
        if (reader.failed()) {
            return;
        }

        if (name.empty()) {
            release_edit_locks(*player);

            return;
        }

        const auto key = to_lower(name);

        if (const auto held = edit_locks_.find(key); held != edit_locks_.end() && held->second == player->state.id) {
            edit_locks_.erase(held);
        }

        player->edit_levels.erase(key);

        if (player->edit_levels.empty()) {
            player->editing = false;
        }
    }

    void server::handle_edit_list_directory(const shared_ptr<session> &player, packet_reader &reader) {
        const auto category = reader.get_u8();
        if (reader.failed() || category > static_cast<uint8_t>(shared::folder_category::shield)) {
            return;
        }

        auto listing = packet_writer(message_id::edit_directory_listing);
        listing.put_u8(category);

        const auto names = player->profile.developer
                               ? world_.list_category(static_cast<shared::folder_category>(category))
                               : vector<string>{};

        listing.put_u32(static_cast<uint32_t>(names.size()));
        for (const auto &name : names) {
            listing.put_string(name);
        }

        player->connection->send(listing);
    }

    void server::handle_edit_fetch_level(const shared_ptr<session> &player, packet_reader &reader) {
        const auto name = reader.get_string();
        if (reader.failed()) {
            return;
        }

        const auto respond = [&](const edit_status status, const vector<uint8_t> &data = {}) {
            player->connection->send(packet_writer(message_id::edit_level_content)
                    .put_string(name)
                    .put_u8(static_cast<uint8_t>(status))
                    .put_bytes(data));
        };

        if (!player->profile.developer) {
            respond(edit_status::not_developer);

            return;
        }

        if (!world_.matches_category(shared::folder_category::level, name)) {
            respond(edit_status::bad_name);

            return;
        }

        if (const auto held = edit_locks_.find(to_lower(name));
            held == edit_locks_.end() || held->second != player->state.id) {
            respond(edit_status::not_locked);

            return;
        }

        const auto path = world_.find_asset(name);
        if (path.empty()) {
            respond(edit_status::not_found);

            return;
        }

        ifstream ifs(path, ios::binary);
        if (!ifs) {
            respond(edit_status::io_error);

            return;
        }

        respond(edit_status::ok, vector<uint8_t>((istreambuf_iterator(ifs)), istreambuf_iterator<char>()));
    }

    void server::handle_edit_save_level(const shared_ptr<session> &player, packet_reader &reader) {
        auto name = reader.get_string();
        const auto data = reader.get_bytes();
        if (reader.failed()) {
            return;
        }

        const string content(data.begin(), data.end());

        const auto respond = [&](const edit_status status, const string_view detail = {}) {
            player->connection->send(packet_writer(message_id::edit_save_result)
                    .put_string(name)
                    .put_u8(static_cast<uint8_t>(status))
                    .put_string(detail));
        };

        if (!player->profile.developer) {
            respond(edit_status::not_developer);

            return;
        }

        if (!world_.matches_category(shared::folder_category::level, name)) {
            respond(edit_status::bad_name);

            return;
        }

        const auto path = world_.level_file_path(name);
        if (const auto existing = world_.find_asset(name); !existing.empty()) {
            name = existing.filename().string();
        }

        const auto key = to_lower(name);

        if (const auto held = edit_locks_.find(key); held != edit_locks_.end() && held->second != player->state.id) {
            for (const auto &other : sessions_) {
                if (other->authenticated && other->state.id == held->second) {
                    respond(edit_status::locked, other->profile.name);

                    return;
                }
            }
        }

        istringstream is(content);
        auto parsed = shared::load_level(name, is);
        if (!parsed) {
            respond(edit_status::invalid, parsed.error());

            return;
        }

        if (parsed->tiles() == nullptr) {
            respond(edit_status::invalid, "level has no board");

            return;
        }

        const auto temporary = filesystem::path(path.string() + ".tmp");

        {
            ofstream ofs(temporary, ios::binary | ios::trunc);
            if (!ofs || !ofs.write(content.data(), static_cast<streamsize>(content.size()))) {
                respond(edit_status::io_error);

                return;
            }
        }

        error_code code;
        filesystem::rename(temporary, path, code);
        if (code) {
            filesystem::remove(temporary, code);
            respond(edit_status::io_error);

            return;
        }

        edit_locks_[key] = player->state.id;

        player->edit_levels.insert(key);
        player->editing = true;

        reload_level(name, make_shared<shared::level_data>(std::move(*parsed)), content);

        respond(edit_status::ok);

        println("[edit] {} saved {}", player->identifier(), name);
    }

    void server::release_edit_locks(session &player) {
        for (const auto &key : player.edit_levels) {
            if (const auto held = edit_locks_.find(key); held != edit_locks_.end() && held->second == player.state.id) {
                edit_locks_.erase(held);
            }
        }

        player.edit_levels.clear();
        player.editing = false;
    }

    void server::reload_level(const string &level_name, shared_ptr<shared::level_data> level, const string &content) {
        world_.replace_level(level_name, std::move(level));

        for (const auto id : scripts_.reset_level(level_name)) {
            send_to_level(level_name, packet_writer(message_id::npc_remove).put_u32(id), nullptr);
        }

        scripts_.ensure_level(level_name);

        auto reload = packet_writer(message_id::level_reload);
        reload.put_string(level_name).put_bytes(vector<uint8_t>(content.begin(), content.end()));
        send_to_level(level_name, reload, nullptr);

        for (const auto &other : sessions_) {
            if (other->authenticated && iequals(other->level, level_name)) {
                send_npcs(other);
            }
        }
    }

    void server::drop(const shared_ptr<session> &player, const string_view reason) {
        player->connection->send(packet_writer(message_id::disconnect).put_string(reason));

        if (player->authenticated) {
            scripts_.player_offline(*player);
            persist(*player);
            leave_level(player);
            release_edit_locks(*player);
        }

        player->connection->close();
    }

    void server::persist(const session &player) const {
        if (!player.authenticated) {
            return;
        }

        auto stored = player.profile;
        stored.level = player.level;
        stored.x = player.state.x;
        stored.y = player.state.y;
        stored.direction = player.state.direction;
        stored.weapons = player.weapons;

        if (const auto saved = save_account(account_path(stored.name), stored); !saved) {
            println(cerr, "could not save account {}: {}", stored.name, saved.error());
        }
    }

    void server::send_to_level(const string &level_name, const packet_writer &writer, const session *except) const {
        if (level_name.empty()) {
            return;
        }

        for (const auto &other : sessions_) {
            if (other.get() == except || other->level != level_name || !other->authenticated) {
                continue;
            }

            other->connection->send(writer);
        }
    }

    auto server::find_by_account(const string_view name) const -> shared_ptr<session> {
        for (const auto &player : sessions_) {
            if (player->authenticated && iequals(player->profile.name, name)) {
                return player;
            }
        }

        return nullptr;
    }

    auto server::account_path(const string_view name) const -> filesystem::path {
        return world_.root() / "accounts" / (encode_container_name(to_lower(name)) + ".txt");
    }

    auto server::next_player_id() -> uint16_t {
        return next_id_++;
    }
}
