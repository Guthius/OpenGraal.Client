#include <catch2/catch_test_macros.hpp>

#include <net/connection.hpp>
#include <server/server.hpp>
#include <shared/container.hpp>
#include <shared/text.hpp>

#include <filesystem>
#include <fstream>
#include <optional>

using namespace std;
using namespace og;
using namespace og::net;

namespace {
    constexpr uint16_t test_port = 15977;

    auto make_world(const filesystem::path &root, const string_view secret = {}) -> void {
        remove_all(root);
        create_directories(root / "levels");
        create_directories(root / "accounts");

        ofstream options(root / "serveroptions.txt");
        options
            << "startlevel=testworld.nw\n"
            << "startx=30\n"
            << "starty=31\n";

        if (!secret.empty()) {
            options << "jwt_secret=" << secret << '\n';
        }

        ofstream(root / "folderconfig.txt") << "level *.nw\n";

        ofstream board(root / "levels" / "testworld.nw");
        board << "GLEVNW01\n";
        for (int y = 0; y < 64; ++y) {
            board << "BOARD 0 " << y << " 64 0 ";
            for (int x = 0; x < 64; ++x) {
                board << "AC";
            }
            board << '\n';
        }
    }

    struct fake_client {
        asio::io_context io;
        connection_ptr link;
        optional<uint16_t> own_id;
        vector<player_state> peers;
        vector<pair<uint16_t, string>> chats;
        vector<uint16_t> departed;
        optional<login_status> result;
        optional<create_status> created;
        string created_message;
        string token;
        string level;
        size_t board_bytes = 0;

        optional<edit_status> edit_result;
        string edit_level;
        string edit_detail;
        vector<string> listing;
        optional<edit_status> fetch_result;
        string fetched;
        optional<edit_status> save_result;
        string save_detail;
        string reloaded_level;
        string reloaded_content;
        vector<uint32_t> npcs_added;
        vector<uint32_t> npcs_removed;

        explicit fake_client(const uint16_t port) {
            link = connection::connect(io, "127.0.0.1", port);
        }

        [[nodiscard]] auto connected() const -> bool {
            return link && link->connected();
        }

        void send(const packet_writer &writer) {
            if (link) {
                link->send(writer);
            }
        }

        void create_account(const string_view name, const string_view password) {
            auto message = packet_writer(message_id::create_account);
            message.put_u16(protocol_version).put_string(name).put_string(password);

            send(message);
        }

        void login(const string_view name, const string_view password, const string_view token = {}) {
            auto message = packet_writer(message_id::login);
            message.put_u16(protocol_version)
                .put_string(name)
                .put_string(password)
                .put_string("")
                .put_string(token);
            send(message);
        }

        void move(const float x, const float y, const uint8_t direction, const string_view animation = "walk") {
            auto message = packet_writer(message_id::move);
            message.put_float(x).put_float(y).put_u8(direction).put_string(animation);
            send(message);
        }

        void say(const string_view text) {
            send(packet_writer(message_id::chat).put_string(text));
        }

        void edit_request(const string_view name) {
            send(packet_writer(message_id::edit_request).put_string(name));
        }

        void edit_release(const string_view name) {
            send(packet_writer(message_id::edit_release).put_string(name));
        }

        void edit_fetch(const string_view name) {
            send(packet_writer(message_id::edit_fetch_level).put_string(name));
        }

        void edit_list(const uint8_t category) {
            send(packet_writer(message_id::edit_list_directory).put_u8(category));
        }

        void edit_save(const string_view name, const string &content) {
            send(packet_writer(message_id::edit_save_level)
                    .put_string(name)
                    .put_bytes(vector<uint8_t>(content.begin(), content.end())));
        }

        void drain() {
            if (!link) {
                return;
            }

            io.restart();
            io.poll();

            while (auto message = link->poll()) {
                switch (message->id()) {
                case message_id::create_account_result:
                    created = static_cast<create_status>(message->get_u8());
                    created_message = message->get_string();
                    break;

                case message_id::login_result:
                    result = static_cast<login_status>(message->get_u8());
                    token = message->get_string();
                    break;

                case message_id::player_identity:
                    own_id = message->get_u16();
                    break;

                case message_id::level_enter:
                    level = message->get_string();
                    break;

                case message_id::level_board:
                    board_bytes = message->get_bytes().size();
                    break;

                case message_id::player_add:
                    peers.push_back(read_player_state(*message));
                    break;

                case message_id::player_update:
                    {
                        const auto state = read_player_state(*message);
                        const auto match = ranges::find(peers, state.id, &player_state::id);

                        if (match == peers.end()) {
                            peers.push_back(state);
                        } else {
                            *match = state;
                        }
                    }
                    break;

                case message_id::player_remove:
                    departed.push_back(message->get_u16());
                    break;

                case message_id::player_chat:
                    {
                        const auto id = message->get_u16();
                        chats.emplace_back(id, message->get_string());
                    }
                    break;

                case message_id::edit_response:
                    edit_level = message->get_string();
                    edit_result = static_cast<edit_status>(message->get_u8());
                    edit_detail = message->get_string();
                    break;

                case message_id::edit_directory_listing:
                    {
                        message->get_u8();
                        const auto count = message->get_u32();

                        listing.clear();
                        for (uint32_t i = 0; i < count; ++i) {
                            listing.push_back(message->get_string());
                        }
                    }
                    break;

                case message_id::edit_level_content:
                    {
                        message->get_string();
                        fetch_result = static_cast<edit_status>(message->get_u8());
                        const auto data = message->get_bytes();
                        fetched = string(data.begin(), data.end());
                    }
                    break;

                case message_id::edit_save_result:
                    message->get_string();
                    save_result = static_cast<edit_status>(message->get_u8());
                    save_detail = message->get_string();
                    break;

                case message_id::level_reload:
                    {
                        reloaded_level = message->get_string();
                        const auto data = message->get_bytes();
                        reloaded_content = string(data.begin(), data.end());
                    }
                    break;

                case message_id::npc_add:
                    npcs_added.push_back(read_npc_state(*message).id);
                    break;

                case message_id::npc_remove:
                    npcs_removed.push_back(message->get_u32());
                    break;

                default:
                    break;
                }
            }
        }
    };

    struct harness {
        filesystem::path root;
        server::server instance;

        explicit harness(const uint16_t port = test_port, const string_view secret = {}, const string_view name = "opengraal-session-test")
            : root(filesystem::temp_directory_path() / name),
              instance(server::server_options{
                  .world_path = (make_world(root, secret), root),
                  .port = port,
              }) {
        }

        ~harness() {
            instance.stop();
            remove_all(root);
        }

        harness(const harness &) = delete;
        auto operator=(const harness &) -> harness & = delete;

        void settle(const initializer_list<fake_client *> clients, const int rounds = 40) {
            for (auto i = 0; i < rounds; ++i) {
                instance.tick(1);

                for (auto *client : clients) {
                    client->drain();
                }
            }
        }
    };

    void make_developer(const filesystem::path &root, const string_view name, const string_view password) {
        server::account dev;
        dev.name = string(name);
        dev.password_hash = server::hash_password(password);
        dev.developer = true;
        dev.level = "testworld.nw";
        dev.x = 30.0f;
        dev.y = 31.0f;

        REQUIRE(save_account(
            root / "accounts" / (og::shared::encode_container_name(og::shared::to_lower(name)) + ".txt"), dev));
    }

    auto board_content(const string_view tile, const string_view extra = {}) -> string {
        auto content = string("GLEVNW01\n");

        for (int y = 0; y < 64; ++y) {
            content += "BOARD 0 " + to_string(y) + " 64 0 ";
            for (int x = 0; x < 64; ++x) {
                content += tile;
            }
            content += '\n';
        }

        content += extra;

        return content;
    }
}

TEST_CASE("a client logs in and receives its level", "[server]") {
    harness world;

    REQUIRE(world.instance.start());

    fake_client alice(test_port);

    REQUIRE(alice.connected());

    alice.login("alice@example.com", "secret");
    world.settle({&alice});

    REQUIRE(alice.result == login_status::ok);
    REQUIRE(alice.own_id.has_value());
    REQUIRE(alice.level == "testworld.nw");
    REQUIRE(alice.board_bytes == 64 * 64 * 2);
}

TEST_CASE("two clients see each other, move and chat", "[server]") {
    harness world;

    REQUIRE(world.instance.start());

    fake_client alice(test_port);
    alice.login("alice@example.com", "secret");
    world.settle({&alice});

    fake_client bob(test_port);
    bob.login("bob@example.com", "hunter2");
    world.settle({&alice, &bob});

    REQUIRE(bob.result == login_status::ok);
    REQUIRE(world.instance.player_count() == 2);

    // Each sees exactly the other.
    REQUIRE(alice.peers.size() == 1);
    REQUIRE(bob.peers.size() == 1);
    REQUIRE(alice.peers[0].id == bob.own_id);
    REQUIRE(bob.peers[0].nickname == "alice");

    // Movement propagates.
    alice.move(32.0f, 33.0f, 3);
    world.settle({&alice, &bob});

    REQUIRE(bob.peers[0].x == 32.0f);
    REQUIRE(bob.peers[0].y == 33.0f);
    REQUIRE(bob.peers[0].direction == 3);
    REQUIRE(bob.peers[0].animation == "walk");

    // Chat reaches everyone in the level, sender included.
    bob.say("hello alice");
    world.settle({&alice, &bob});

    REQUIRE(alice.chats.size() == 1);
    REQUIRE(alice.chats[0].first == bob.own_id);
    REQUIRE(alice.chats[0].second == "hello alice");
    REQUIRE(bob.chats.size() == 1);
}

TEST_CASE("leaving removes the player for everyone else", "[server]") {
    harness world;

    REQUIRE(world.instance.start());

    fake_client alice(test_port);
    alice.login("alice@example.com", "secret");
    world.settle({&alice});

    auto bob = make_optional<fake_client>(test_port);
    bob->login("bob@example.com", "hunter2");
    world.settle({&alice, &*bob});

    const auto bob_id = *bob->own_id;

    bob.reset();
    world.settle({&alice});

    REQUIRE(alice.departed.size() == 1);
    REQUIRE(alice.departed[0] == bob_id);
    REQUIRE(world.instance.player_count() == 1);
}

TEST_CASE("a wrong password is refused once the account exists", "[server]") {
    harness world;

    REQUIRE(world.instance.start());

    fake_client first(test_port);
    first.login("carol@example.com", "correct");
    world.settle({&first});

    REQUIRE(first.result == login_status::ok);

    first.link->close();
    world.settle({&first});

    fake_client impostor(test_port);
    impostor.login("carol@example.com", "guess");
    world.settle({&impostor});

    REQUIRE(impostor.result == login_status::bad_credentials);
}

TEST_CASE("the same account cannot be online twice", "[server]") {
    harness world;

    REQUIRE(world.instance.start());

    fake_client first(test_port);
    first.login("dave@example.com", "secret");
    world.settle({&first});

    fake_client second(test_port);
    second.login("dave@example.com", "secret");
    world.settle({&first, &second});

    REQUIRE(first.result == login_status::ok);
    REQUIRE(second.result == login_status::already_online);
}

TEST_CASE("a teleport is rejected and the client is corrected", "[server]") {
    harness world;

    REQUIRE(world.instance.start());

    fake_client alice(test_port);
    alice.login("alice@example.com", "secret");
    world.settle({&alice});

    fake_client bob(test_port);
    bob.login("bob@example.com", "hunter2");
    world.settle({&alice, &bob});

    bob.peers.clear();
    alice.peers.clear();
    alice.move(500.0f, 500.0f, 0);
    world.settle({&alice, &bob});

    // Nobody else saw the jump, and the mover was told where it really is.
    REQUIRE(bob.peers.empty());
    REQUIRE(alice.peers.size() == 1);
    REQUIRE(alice.peers[0].id == alice.own_id);
    REQUIRE(alice.peers[0].x < 500.0f);
}

TEST_CASE("a warp landing inside the return link does not bounce the player back", "[server]") {
    harness world;

    // A two-way door: b's destination lands inside a's own link rectangle.
    ofstream(world.root / "levels" / "linka.nw") << board_content("AC", "LINK linkb.nw 10 10 2 2 40 40\n");
    ofstream(world.root / "levels" / "linkb.nw") << board_content("AC", "LINK linka.nw 40 40 2 2 11 11\n");

    server::account traveller;
    traveller.name = "alice@example.com";
    traveller.password_hash = server::hash_password("secret");
    traveller.level = "linka.nw";
    traveller.x = 9.0f;
    traveller.y = 11.0f;

    REQUIRE(save_account(
        world.root / "accounts" / (og::shared::encode_container_name("alice@example.com") + ".txt"), traveller));

    REQUIRE(world.instance.start());

    fake_client alice(test_port);
    alice.login("alice@example.com", "secret");
    world.settle({&alice});
    REQUIRE(alice.level == "linka.nw");

    alice.move(11.0f, 11.0f, 0);
    world.settle({&alice});
    REQUIRE(alice.level == "linkb.nw");

    // The arrival point sits on b's link back to a; milling around on it must not warp.
    alice.move(40.5f, 40.5f, 0);
    world.settle({&alice});
    alice.move(41.0f, 40.0f, 0);
    world.settle({&alice});
    REQUIRE(alice.level == "linkb.nw");

    // Stepping off re-arms the link, and walking back onto it takes it.
    alice.move(44.0f, 40.0f, 0);
    world.settle({&alice});
    alice.move(41.0f, 41.0f, 0);
    world.settle({&alice});
    REQUIRE(alice.level == "linka.nw");
}

TEST_CASE("a link on a blocked tile is taken by touch", "[server]") {
    harness world;

    // A doorway one tile high on a wall row: the player can stand under it, never inside it.
    ofstream(world.root / "levels" / "linkc.nw") << board_content("AC", "LINK linkd.nw 20 19 2 1 30 30\n");
    ofstream(world.root / "levels" / "linkd.nw") << board_content("AC");

    server::account traveller;
    traveller.name = "alice@example.com";
    traveller.password_hash = server::hash_password("secret");
    traveller.level = "linkc.nw";
    traveller.x = 20.0f;
    traveller.y = 24.0f;

    REQUIRE(save_account(
        world.root / "accounts" / (og::shared::encode_container_name("alice@example.com") + ".txt"), traveller));

    REQUIRE(world.instance.start());

    fake_client alice(test_port);
    alice.login("alice@example.com", "secret");
    world.settle({&alice});
    REQUIRE(alice.level == "linkc.nw");

    // Standing right under the door but facing away: not touching it.
    alice.move(20.0f, 20.0f, 2);
    world.settle({&alice});
    REQUIRE(alice.level == "linkc.nw");

    // Turning to face the door touches it, and the link takes the player.
    alice.move(20.0f, 20.0f, 0);
    world.settle({&alice});
    REQUIRE(alice.level == "linkd.nw");
}

TEST_CASE("position survives a reconnect", "[server]") {
    harness world;

    REQUIRE(world.instance.start());

    {
        fake_client alice(test_port);
        alice.login("erin@example.com", "secret");
        world.settle({&alice});

        alice.move(33.0f, 34.0f, 1);
        world.settle({&alice});
    }

    world.settle({});

    fake_client again(test_port);
    again.login("erin@example.com", "secret");
    world.settle({&again});

    REQUIRE(again.result == login_status::ok);
    REQUIRE(again.peers.empty());
}

TEST_CASE("files are served from the world and traversal is refused", "[server]") {
    harness world;

    REQUIRE(world.instance.start());

    fake_client alice(test_port);
    alice.login("alice@example.com", "secret");
    world.settle({&alice});

    alice.send(packet_writer(message_id::request_file).put_string("testworld.nw"));

    string served;
    auto missing = false;

    for (auto i = 0; i < 40 && served.empty() && !missing; ++i) {
        world.instance.tick(1);
        alice.io.restart();
        alice.io.poll();

        while (auto message = alice.link->poll()) {
            if (message->id() == message_id::file_data) {
                message->get_string();
                const auto data = message->get_bytes();
                served.assign(data.begin(), data.end());
            } else if (message->id() == message_id::file_missing) {
                missing = true;
            }
        }
    }

    REQUIRE_FALSE(missing);
    REQUIRE(served.starts_with("GLEVNW01"));

    alice.send(packet_writer(message_id::request_file).put_string("../serveroptions.txt"));

    auto refused = false;
    for (auto i = 0; i < 40 && !refused; ++i) {
        world.instance.tick(1);
        alice.io.restart();
        alice.io.poll();

        while (auto message = alice.link->poll()) {
            if (message->id() == message_id::file_missing) {
                refused = true;
            }
        }
    }

    REQUIRE(refused);
}

TEST_CASE("a disconnect saves the level the player was standing in", "[server]") {
    harness world;

    REQUIRE(world.instance.start());

    {
        fake_client alice(test_port);
        alice.login("erin@example.com", "secret");
        world.settle({&alice});
    }

    world.settle({});

    const auto stored = og::server::load_account(world.root / "accounts" / "erin@example.com.txt");

    REQUIRE(stored);
    REQUIRE(stored->level == "testworld.nw");
}

TEST_CASE("an account can be created and then logged into", "[server]") {
    harness world;

    REQUIRE(world.instance.start());

    fake_client maker(test_port);
    maker.create_account("frank@example.com", "hunter2");
    world.settle({&maker});

    REQUIRE(maker.created == create_status::ok);

    // Written straight away, not deferred to disconnect the way auto-registration is.
    REQUIRE(exists(world.root / "accounts" / "frank@example.com.txt"));

    maker.login("frank@example.com", "hunter2");
    world.settle({&maker});

    REQUIRE(maker.result == login_status::ok);
}

TEST_CASE("account creation refuses a duplicate, a bad address and a weak password", "[server]") {
    harness world;

    REQUIRE(world.instance.start());

    fake_client maker(test_port);
    maker.create_account("grace@example.com", "hunter2");
    world.settle({&maker});

    REQUIRE(maker.created == create_status::ok);

    maker.create_account("grace@example.com", "hunter2");
    world.settle({&maker});

    REQUIRE(maker.created == create_status::already_exists);

    maker.create_account("not-an-address", "hunter2");
    world.settle({&maker});

    REQUIRE(maker.created == create_status::bad_email);

    maker.create_account("henry@example.com", "short");
    world.settle({&maker});

    REQUIRE(maker.created == create_status::weak_password);
}

TEST_CASE("an unauthenticated session may create an account but nothing else", "[server]") {
    harness world;

    REQUIRE(world.instance.start());

    fake_client intruder(test_port);
    intruder.move(10.0f, 10.0f, 2, "idle.gani");
    world.settle({&intruder});

    REQUIRE_FALSE(intruder.connected());
}

TEST_CASE("a token logs a player in without their password", "[server]") {
    harness world(test_port, "a-test-secret-of-at-least-32-chars");

    REQUIRE(world.instance.start());

    fake_client first(test_port);
    first.login("ivy@example.com", "hunter2");
    world.settle({&first});

    REQUIRE(first.result == login_status::ok);
    REQUIRE_FALSE(first.token.empty());

    const auto token = first.token;

    first.link->close();
    world.settle({&first});

    fake_client again(test_port);
    again.login("ivy@example.com", "", token);
    world.settle({&again});

    REQUIRE(again.result == login_status::ok);
}

TEST_CASE("a token minted by another world is refused", "[server]") {
    harness world(test_port, "a-test-secret-of-at-least-32-chars");
    harness other(test_port + 1, "a-different-secret-also-32-chars-long", "opengraal-session-other");

    REQUIRE(world.instance.start());
    REQUIRE(other.instance.start());

    fake_client elsewhere(test_port + 1);
    elsewhere.login("ivy@example.com", "hunter2");
    other.settle({&elsewhere});

    REQUIRE(elsewhere.result == login_status::ok);
    REQUIRE_FALSE(elsewhere.token.empty());

    fake_client crossing(test_port);
    crossing.login("ivy@example.com", "", elsewhere.token);
    world.settle({&crossing});

    REQUIRE(crossing.result == login_status::bad_credentials);
}

TEST_CASE("a token for one account cannot log in another", "[server]") {
    harness world(test_port, "a-test-secret-of-at-least-32-chars");

    REQUIRE(world.instance.start());

    fake_client first(test_port);
    first.login("judy@example.com", "hunter2");
    world.settle({&first});

    REQUIRE(first.result == login_status::ok);

    fake_client impostor(test_port);
    impostor.login("karl@example.com", "", first.token);
    world.settle({&impostor});

    REQUIRE(impostor.result == login_status::bad_credentials);
}

TEST_CASE("editing needs the developer flag", "[server]") {
    harness world;

    make_developer(world.root, "dev@example.com", "secret123");

    REQUIRE(world.instance.start());

    fake_client norm(test_port);
    norm.login("norm@example.com", "secret123");
    world.settle({&norm});

    norm.edit_request("");
    world.settle({&norm});

    REQUIRE(norm.edit_result == edit_status::not_developer);

    fake_client dev(test_port);
    dev.login("dev@example.com", "secret123");
    world.settle({&dev});

    REQUIRE(dev.result == login_status::ok);

    dev.edit_request("");
    world.settle({&dev});

    REQUIRE(dev.edit_result == edit_status::ok);
    REQUIRE(dev.edit_level == "testworld.nw");
}

TEST_CASE("the edit lock is exclusive, names its holder, and frees on disconnect", "[server]") {
    harness world;

    make_developer(world.root, "dev@example.com", "secret123");
    make_developer(world.root, "dev2@example.com", "secret123");

    REQUIRE(world.instance.start());

    auto first = make_optional<fake_client>(test_port);
    first->login("dev@example.com", "secret123");

    fake_client second(test_port);
    second.login("dev2@example.com", "secret123");
    world.settle({&*first, &second});

    first->edit_request("");
    world.settle({&*first, &second});

    REQUIRE(first->edit_result == edit_status::ok);

    second.edit_request("testworld.nw");
    world.settle({&*first, &second});

    REQUIRE(second.edit_result == edit_status::locked);
    REQUIRE(second.edit_detail == "dev@example.com");

    second.edit_save("testworld.nw", board_content("AB"));
    world.settle({&*first, &second});

    REQUIRE(second.save_result == edit_status::locked);

    first->edit_request("");
    world.settle({&*first, &second});

    REQUIRE(first->edit_result == edit_status::ok);

    first.reset();
    world.settle({&second});

    second.edit_request("testworld.nw");
    world.settle({&second});

    REQUIRE(second.edit_result == edit_status::ok);
}

TEST_CASE("an editing player is frozen until they release", "[server]") {
    harness world;

    make_developer(world.root, "dev@example.com", "secret123");

    REQUIRE(world.instance.start());

    fake_client dev(test_port);
    dev.login("dev@example.com", "secret123");

    fake_client bob(test_port);
    bob.login("bob@example.com", "secret123");
    world.settle({&dev, &bob});

    REQUIRE(dev.own_id.has_value());

    dev.move(33.0f, 31.0f, 1);
    world.settle({&dev, &bob});

    const auto seen = [&]() -> const player_state * {
        const auto match = ranges::find(bob.peers, *dev.own_id, &player_state::id);
        return match == bob.peers.end() ? nullptr : &*match;
    };

    REQUIRE(seen() != nullptr);
    REQUIRE(seen()->x == 33.0f);

    dev.edit_request("");
    world.settle({&dev, &bob});

    REQUIRE(dev.edit_result == edit_status::ok);

    dev.move(36.0f, 31.0f, 1);
    world.settle({&dev, &bob});

    REQUIRE(seen()->x == 33.0f);

    dev.edit_release("");
    dev.move(36.0f, 31.0f, 1);
    world.settle({&dev, &bob});

    REQUIRE(seen()->x == 36.0f);
}

TEST_CASE("a save must parse and its name must fit the level category", "[server]") {
    harness world;

    make_developer(world.root, "dev@example.com", "secret123");

    REQUIRE(world.instance.start());

    fake_client dev(test_port);
    dev.login("dev@example.com", "secret123");
    world.settle({&dev});

    dev.edit_request("");
    world.settle({&dev});

    REQUIRE(dev.edit_result == edit_status::ok);

    const auto before = [&world] {
        ifstream ifs(world.root / "levels" / "testworld.nw", ios::binary);
        return string((istreambuf_iterator(ifs)), istreambuf_iterator<char>());
    }();

    dev.edit_save("testworld.nw", "this is not a level");
    world.settle({&dev});

    REQUIRE(dev.save_result == edit_status::invalid);

    dev.save_result.reset();
    dev.edit_save("testworld.nw", "GLEVNW01\nNPC - 4 5\nNPCEND\n");
    world.settle({&dev});

    REQUIRE(dev.save_result == edit_status::invalid);

    dev.save_result.reset();
    dev.edit_save("../evil.nw", board_content("AB"));
    world.settle({&dev});

    REQUIRE(dev.save_result == edit_status::bad_name);

    dev.save_result.reset();
    dev.edit_save("evil.png", board_content("AB"));
    world.settle({&dev});

    REQUIRE(dev.save_result == edit_status::bad_name);
    REQUIRE_FALSE(exists(world.root / "levels" / "evil.png"));

    ifstream ifs(world.root / "levels" / "testworld.nw", ios::binary);
    REQUIRE(string((istreambuf_iterator(ifs)), istreambuf_iterator<char>()) == before);
}

TEST_CASE("a save hot-reloads the level for everyone standing in it", "[server]") {
    harness world;

    make_developer(world.root, "dev@example.com", "secret123");

    REQUIRE(world.instance.start());

    fake_client dev(test_port);
    dev.login("dev@example.com", "secret123");

    fake_client bob(test_port);
    bob.login("bob@example.com", "secret123");
    world.settle({&dev, &bob});

    dev.edit_request("");
    world.settle({&dev, &bob});

    REQUIRE(dev.edit_result == edit_status::ok);

    const auto with_npc = board_content("AB", "NPC - 10 10\nfunction onCreated() {}\nNPCEND\n");

    dev.edit_save("testworld.nw", with_npc);
    world.settle({&dev, &bob});

    REQUIRE(dev.save_result == edit_status::ok);
    REQUIRE(bob.reloaded_level == "testworld.nw");
    REQUIRE(bob.reloaded_content == with_npc);
    REQUIRE(bob.npcs_added.size() == 1);

    const auto npc_id = bob.npcs_added.front();

    dev.edit_save("testworld.nw", board_content("AB"));
    world.settle({&dev, &bob});

    REQUIRE(ranges::contains(bob.npcs_removed, npc_id));

    dev.edit_fetch("testworld.nw");
    world.settle({&dev});

    REQUIRE(dev.fetch_result == edit_status::ok);
    REQUIRE(dev.fetched == board_content("AB"));

    dev.edit_save("brandnew.nw", board_content("AC"));
    world.settle({&dev});

    REQUIRE(dev.save_result == edit_status::ok);
    REQUIRE(exists(world.root / "levels" / "brandnew.nw"));
}

TEST_CASE("directory listings answer only for developers and their categories", "[server]") {
    harness world;

    make_developer(world.root, "dev@example.com", "secret123");

    REQUIRE(world.instance.start());

    fake_client norm(test_port);
    norm.login("norm@example.com", "secret123");

    fake_client dev(test_port);
    dev.login("dev@example.com", "secret123");
    world.settle({&norm, &dev});

    dev.edit_list(1);
    norm.edit_list(1);
    world.settle({&norm, &dev});

    REQUIRE(dev.listing == vector<string>{"testworld.nw"});
    REQUIRE(norm.listing.empty());

    dev.edit_list(0);
    world.settle({&dev});

    REQUIRE(dev.listing.empty());
}
