#include <catch2/catch_test_macros.hpp>

#include <server/script_host.hpp>
#include <server/server.hpp>

#include <filesystem>
#include <fstream>

using namespace std;
using namespace og;
using namespace og::gs2;

namespace {
    struct scripted_world {
        filesystem::path root = filesystem::temp_directory_path() / "opengraal-script-test";
        server::world game_world;

        scripted_world() {
            remove_all(root);
            create_directories(root / "levels");
            create_directories(root / "npcs");
            create_directories(root / "weapons");
            create_directories(root / "scripts");

            ofstream(root / "serveroptions.txt") << "startlevel=testworld.nw\nstartx=30\nstarty=30\n";
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

        ~scripted_world() {
            remove_all(root);
        }

        scripted_world(const scripted_world &) = delete;
        auto operator=(const scripted_world &) -> scripted_world & = delete;

        void add_npc(const string_view name, const string_view script) const {
            ofstream(root / "npcs" / (string(name) + ".txt"))
                << "GRNPC001\nNAME " << name << "\nID 1000\nIMAGE \nLEVEL testworld.nw\nX 30\nY 30\nNPCSCRIPT\n"
                << script << "\nNPCSCRIPTEND\n";
        }

        void add_weapon(const string_view name, const string_view script) const {
            ofstream(root / "weapons" / (string(name) + ".txt"))
                << "GRAWP001\nREALNAME " << name << "\nIMAGE \nSCRIPT\n"
                << script << "\nSCRIPTEND\n";
        }

        void add_class(const string_view name, const string_view script) const {
            ofstream(root / "scripts" / (string(name) + ".txt")) << script << '\n';
        }

        auto load() -> server::script_host {
            REQUIRE(game_world.load(root));

            auto host = server::script_host(game_world);
            host.load();

            return host;
        }
    };

    auto make_session() -> shared_ptr<server::session> {
        auto player = make_shared<server::session>();

        player->authenticated = true;
        player->level = "testworld.nw";
        player->profile.name = "alice";
        player->state.id = 7;
        player->state.nickname = "alice";
        player->state.x = 30.0f;
        player->state.y = 30.0f;

        return player;
    }
}

TEST_CASE("an npc greets an entering player", "[scripts]") {
    scripted_world world;

    world.add_npc("greeter", R"(
        function onCreated() {
            this.nick = "Greeter";
            this.greeted = 0;
        }

        function onPlayerEnters() {
            this.greeted = this.greeted + 1;
            this.chat = "Hello" SPC player.nick @ "! #" @ this.greeted;
        }
    )");

    auto host = world.load();

    string heard;
    host.on_chat([&heard](const server::npc &, const string &text) { heard = text; });

    host.ensure_level("testworld.nw");
    host.tick(1.0);

    auto player = make_session();
    host.player_entered(*player);

    REQUIRE(heard == "Hello alice! #1");

    host.player_entered(*player);

    REQUIRE(heard == "Hello alice! #2");
}

TEST_CASE("npc state changes reach the wire", "[scripts]") {
    scripted_world world;

    world.add_npc("mover", R"(
        function onPlayerEnters() {
            this.x = 42;
            this.y = 43;
            this.nick = "Moved";
        }
    )");

    auto host = world.load();
    host.tick(1.0);

    auto player = make_session();
    host.player_entered(*player);

    const auto npcs = host.npcs_in("testworld.nw");

    REQUIRE(npcs.size() == 1);
    REQUIRE(npcs[0]->state.x == 42.0f);
    REQUIRE(npcs[0]->state.y == 43.0f);
    REQUIRE(npcs[0]->state.nickname == "Moved");
    REQUIRE(npcs[0]->dirty);
}

TEST_CASE("onCreated runs once and onTimeout repeats on the tick", "[scripts]") {
    scripted_world world;

    world.add_npc("ticker", R"(
        function onCreated() {
            this.creations = this.creations + 1;
            this.ticks = 0;
            this.timeout = 0.1;
        }

        function onTimeout() {
            this.ticks = this.ticks + 1;
            this.timeout = 0.1;
        }
    )");

    auto host = world.load();

    for (auto i = 0; i < 5; ++i) {
        host.tick(0.1);
    }

    const auto npcs = host.npcs_in("testworld.nw");

    REQUIRE(npcs.size() == 1);
    REQUIRE(to_number(npcs[0]->self->get("creations")) == 1.0);
    REQUIRE(to_number(npcs[0]->self->get("ticks")) >= 3.0);
}

TEST_CASE("a chatting player reaches onPlayerChats", "[scripts]") {
    scripted_world world;

    world.add_npc("listener", R"(
        function onPlayerChats() {
            this.chat = "you said" SPC params[0];
        }
    )");

    auto host = world.load();

    string heard;
    host.on_chat([&heard](const server::npc &, const string &text) { heard = text; });

    host.tick(1.0);

    auto player = make_session();
    host.player_chatted(*player, "hello there");

    REQUIRE(heard == "you said hello there");
}

TEST_CASE("triggerserver reaches a public weapon handler", "[scripts]") {
    scripted_world world;

    world.add_weapon("Hud", R"(
        public function onActionServerside() {
            server.last_action = params[0];
            server.last_argument = params[1];
        }
    )");

    auto host = world.load();

    auto player = make_session();
    host.trigger_server(*player, "Hud", "ready", {value{string("payload")}});

    const auto *entry = host.find_weapon("Hud");

    REQUIRE(entry != nullptr);
    REQUIRE(entry->script != nullptr);
    REQUIRE(entry->script->is_public("onActionServerside"));
}

TEST_CASE("handlers declared in Thamhic's casing still dispatch", "[scripts]") {
    scripted_world world;

    world.add_weapon("Commands", R"(
        public function onActionserverside() {
            player.setvisible(0);
        }
    )");

    world.add_npc("pixie", R"(
        function onPlayerchats() {
            this.chat = "heard" SPC params[0];
        }
    )");

    auto host = world.load();

    string heard;
    host.on_chat([&heard](const server::npc &, const string &text) { heard = text; });

    host.tick(1.0);

    auto player = make_session();

    REQUIRE(player->state.visible);

    host.trigger_server(*player, "Commands", "ready", {});
    host.player_chatted(*player, "hello");

    REQUIRE(host.find_weapon("Commands")->script->is_public("onActionServerside"));
    REQUIRE_FALSE(player->state.visible);
    REQUIRE(heard == "heard hello");
}

TEST_CASE("a positional action reaches only the npcs standing on it", "[scripts]") {
    scripted_world world;

    world.add_npc("near", R"(
        function onActionGrab() {
            this.chat = "grabbed" SPC params[0];
        }
    )");

    auto host = world.load();
    host.tick(1.0);

    auto player = make_session();

    host.trigger_action(*player, "testworld.nw", 30.5f, 30.5f, "Grab", {value{string("hard")}});

    const auto &npcs = host.get_npcs();

    REQUIRE(npcs.size() == 1);
    REQUIRE(npcs.front()->state.chat == "grabbed hard");

    npcs.front()->self->put("chat", value{string()});
    host.trigger_action(*player, "testworld.nw", 48.0f, 48.0f, "Grab", {value{string("hard")}});

    REQUIRE(npcs.front()->self->get("chat") == value{string()});
}

TEST_CASE("a positional action can only ever name an onAction handler", "[scripts]") {
    scripted_world world;

    world.add_npc("guarded", R"(
        function onCreated() {
            this.chat = "created";
        }

        function secret() {
            this.chat = "reached";
        }
    )");

    auto host = world.load();
    host.tick(1.0);

    auto player = make_session();

    // Whatever the client sends is prefixed, so no bare function and no other event is in reach.
    host.trigger_action(*player, "testworld.nw", 30.5f, 30.5f, "secret", {});
    host.trigger_action(*player, "testworld.nw", 30.5f, 30.5f, "Created", {});

    REQUIRE(host.get_npcs().front()->self->get("chat") == value{string("created")});
}

TEST_CASE("a serverside script's triggeraction reaches the player who caused it", "[scripts]") {
    scripted_world world;

    world.add_weapon("Profile", R"(
        public function onActionServerside() {
            triggeraction(0, 0, "clientside", "Systems/Profile", "getprofile", player.account);
        }
    )");

    auto host = world.load();

    string weapon;
    string action;
    values arguments;

    host.on_trigger_client([&](const net::connection_ptr &, const string &name, const string &sent, const values &args) {
        weapon = name;
        action = sent;
        arguments = args;
    });

    auto player = make_session();
    host.trigger_server(*player, "Profile", "ready", {});

    REQUIRE(weapon == "Systems/Profile");
    REQUIRE(action == "getprofile");
    REQUIRE(arguments.size() == 1);
    REQUIRE(to_string(arguments.front()) == "alice");
}

TEST_CASE("what a script writes on the player sticks", "[scripts]") {
    scripted_world world;

    world.add_weapon("Dress", R"(
        public function onActionServerside() {
            player.headimg = "thamhic_head1.png";
            player.bodyimg = "thamhic_body1.png";
            player.nick = "Ajira";
            player.attr[4] = "thamhic_hairs1.png";
        }
    )");

    auto host = world.load();

    auto player = make_session();
    host.trigger_server(*player, "Dress", "serversideinit", {});

    REQUIRE(player->state.head == "thamhic_head1.png");
    REQUIRE(player->state.body == "thamhic_body1.png");
    REQUIRE(player->state.nickname == "Ajira");
    REQUIRE(player->state.attr[4] == "thamhic_hairs1.png");
    REQUIRE(player->state_changed);
}

TEST_CASE("a player property survives from one dispatch to the next", "[scripts]") {
    scripted_world world;

    world.add_weapon("Remember", R"(
        public function onActionServerside() {
            if (params[0] == "set") {
                player.headimg = "one.png";
            } else {
                server.seen = player.headimg;
            }
        }
    )");

    auto host = world.load();

    auto player = make_session();
    host.trigger_server(*player, "Remember", "set", {});
    host.trigger_server(*player, "Remember", "read", {});

    REQUIRE(to_string(host.get_server_flags()->get("seen")) == "one.png");
}

TEST_CASE("the server keeps account, id and level to itself", "[scripts]") {
    scripted_world world;

    world.add_weapon("Forge", R"(
        public function onActionServerside() {
            player.account = "root";
            player.id = 1;
            player.level = "elsewhere.nw";
        }
    )");

    auto host = world.load();

    auto player = make_session();
    host.trigger_server(*player, "Forge", "go", {});

    REQUIRE(player->profile.name == "alice");
    REQUIRE(player->state.id == 7);
    REQUIRE(player->level == "testworld.nw");
}

TEST_CASE("onActionServerside runs without public, because the client cannot name a function", "[scripts]") {
    scripted_world world;

    world.add_weapon("Unmarked", R"(
        function onActionServerside() {
            server.reached = params[0];
        }
    )");

    auto host = world.load();

    auto player = make_session();
    host.trigger_server(*player, "Unmarked", "ready", {});

    REQUIRE_FALSE(host.find_weapon("Unmarked")->script->is_public("onActionServerside"));
    REQUIRE(to_string(host.get_server_flags()->get("reached")) == "ready");
}

TEST_CASE("public still gates a script naming a function on another object", "[scripts]") {
    scripted_world world;

    world.add_weapon("Locked", R"(
        function hidden() {
            return 1;
        }

        public function open() {
            return 1;
        }
    )");

    auto host = world.load();

    const auto *entry = host.find_weapon("Locked");
    auto self = make_shared<gs2::basic_dictionary>();

    REQUIRE(entry->script->call_public("open", self));
    REQUIRE_FALSE(entry->script->call_public("hidden", self));
}

TEST_CASE("the clientside half is kept for pushing, not run on the server", "[scripts]") {
    scripted_world world;

    world.add_weapon("Split", R"(
        public function onActionServerside() {
            this.server_side = 1;
        }
        //#CLIENTSIDE
        function onCreated() {
            this.client_side = 1;
        }
    )");

    auto host = world.load();

    const auto *entry = host.find_weapon("Split");

    REQUIRE(entry != nullptr);
    REQUIRE(entry->client_script.contains("client_side"));
    REQUIRE_FALSE(entry->client_script.contains("server_side"));
    REQUIRE(entry->script->has_function("onActionServerside"));
    REQUIRE_FALSE(entry->script->has_function("onCreated"));
}

TEST_CASE("npcs can join class scripts from the world", "[scripts]") {
    scripted_world world;

    world.add_class("helpers", "function greet() { return \"from the class\"; }");
    world.add_npc("joiner", R"(
        function onCreated() {
            join("helpers");
            this.chat = greet();
        }
    )");

    auto host = world.load();

    string heard;
    host.on_chat([&heard](const server::npc &, const string &text) { heard = text; });

    host.tick(1.0);

    REQUIRE(heard == "from the class");
}

TEST_CASE("a failing script does not stop the tick", "[scripts]") {
    scripted_world world;

    world.add_npc("broken", R"(
        function onCreated() {
            temp = 1;
        }
    )");

    world.add_npc("healthy", R"(
        function onCreated() {
            this.chat = "still here";
        }
    )");

    auto host = world.load();

    string heard;
    host.on_chat([&heard](const server::npc &, const string &text) { heard = text; });

    host.tick(1.0);

    REQUIRE(heard == "still here");
}

TEST_CASE("two npcs from one class script keep separate locals", "[scripts]") {
    scripted_world world;

    world.add_npc("first", R"(
        function onCreated() {
            counter = counter + 1;
            this.chat = "first" SPC counter;
        }
    )");

    world.add_npc("second", R"(
        function onCreated() {
            counter = counter + 10;
            this.chat = "second" SPC counter;
        }
    )");

    auto host = world.load();

    vector<string> heard;
    host.on_chat([&heard](const server::npc &, const string &text) { heard.push_back(text); });

    host.tick(1.0);

    REQUIRE(heard.size() == 2);
    REQUIRE(heard[0] == "first 1");
    REQUIRE(heard[1] == "second 10");
}

TEST_CASE("a script can hide a player and warp them to another server", "[scripts]") {
    scripted_world world;

    world.add_weapon("Warp", R"(
        public function onActionServerside() {
            if (params[0] == "hide") {
                player.setvisible(0);
            }

            if (params[0] == "here") {
                player.serverwarp();
            }

            if (params[0] == "host") {
                player.serverwarp("10.0.0.1");
            }

            if (params[0] == "hostport") {
                player.serverwarp("10.0.0.1", 14901);
            }
        }
    )");

    auto host = world.load();

    string warped_host;
    uint16_t warped_port = 0;
    string warped_account;
    auto warps = 0;

    host.on_server_warp([&](const net::connection_ptr &, const string &target, const uint16_t port, const string &account) {
        warped_host = target;
        warped_port = port;
        warped_account = account;
        ++warps;
    });

    auto player = make_session();

    REQUIRE(player->state.visible);

    host.trigger_server(*player, "Warp", "hide", {});

    REQUIRE_FALSE(player->state.visible);

    // No arguments means "reconnect to this world": no host, no port.
    host.trigger_server(*player, "Warp", "here", {});

    REQUIRE(warps == 1);
    REQUIRE(warped_host.empty());
    REQUIRE(warped_port == 0);
    REQUIRE(warped_account == player->profile.name);

    host.trigger_server(*player, "Warp", "host", {});

    REQUIRE(warped_host == "10.0.0.1");
    REQUIRE(warped_port == 0);

    host.trigger_server(*player, "Warp", "hostport", {});

    REQUIRE(warped_host == "10.0.0.1");
    REQUIRE(warped_port == 14901);
}

TEST_CASE("serverwarp no longer resolves a world name through serveroptions", "[scripts]") {
    scripted_world world;

    world.add_weapon("Warp", R"(
        public function onActionServerside() {
            player.serverwarp("other");
        }
    )");

    auto host = world.load();

    string warped_host;
    uint16_t warped_port = 0;

    host.on_server_warp([&](const net::connection_ptr &, const string &target, const uint16_t port, const string &) {
        warped_host = target;
        warped_port = port;
    });

    auto player = make_session();

    host.trigger_server(*player, "Warp", "go", {});

    // The name is passed through as a host; nothing looks it up any more.
    REQUIRE(warped_host == "other");
    REQUIRE(warped_port == 0);
}

TEST_CASE("addweapon and removeweapon tell the server what to push", "[scripts]") {
    scripted_world world;

    world.add_weapon("Granter", R"(
        public function onActionServerside() {
            if (params[0] == "give") {
                addweapon("Systems/Gui");
            }

            if (params[0] == "take") {
                removeweapon("Systems/Gui");
            }
        }
    )");

    world.add_weapon("Systems/Gui", R"(
        //#CLIENTSIDE
        function onCreated() {
        }
    )");

    auto host = world.load();

    vector<pair<string, bool>> changes;

    host.on_weapon_changed([&](server::session &, const string &name, const bool granted) {
        changes.emplace_back(name, granted);
    });

    auto player = make_session();

    host.trigger_server(*player, "Granter", "give", {});

    REQUIRE(player->weapons == vector<string>{"Systems/Gui"});
    REQUIRE(changes == vector<pair<string, bool>>{
                           {"Systems/Gui", true}
    });

    host.trigger_server(*player, "Granter", "give", {});

    REQUIRE(player->weapons == vector<string>{"Systems/Gui"});
    REQUIRE(changes.size() == 2);

    host.trigger_server(*player, "Granter", "take", {});

    REQUIRE(player->weapons.empty());
    REQUIRE(changes.back() == pair<string, bool>{"Systems/Gui", false});
}

TEST_CASE("a class joined onto a player becomes the player's own functions", "[scripts]") {
    scripted_world world;

    world.add_class("greeter", R"(
        public function Greet() {
            this.greeting = "hello " @ player.account;

            return this.greeting;
        }
    )");

    world.add_weapon("Joiner", R"(
        public function onActionServerside() {
            if (params[0] == "join") {
                player.join("greeter");
            }

            if (params[0] == "call") {
                serverr.said = player.Greet();
                serverr.classes = player.joinedclasses;
            }
        }
    )");

    auto host = world.load();
    auto player = make_session();

    host.trigger_server(*player, "Joiner", "join", {});

    REQUIRE(player->joined_classes == vector<string>{"greeter"});

    host.trigger_server(*player, "Joiner", "call", {});

    REQUIRE(to_string(host.get_server_flags()->get("said")) == "hello alice");
    REQUIRE(to_string(host.get_server_flags()->get("classes")) == "greeter");
}

TEST_CASE("setlevel2 reports where a script wants the player", "[scripts]") {
    scripted_world world;

    world.add_weapon("Mover", R"(
        public function onActionServerside() {
            player.setlevel2("start.nw", 30, 31);
        }
    )");

    auto host = world.load();

    string moved_level;
    float moved_x = 0.0f;
    float moved_y = 0.0f;

    host.on_set_level([&](server::session &, const string &level, const float x, const float y) {
        moved_level = level;
        moved_x = x;
        moved_y = y;
    });

    auto player = make_session();
    host.trigger_server(*player, "Mover", "go", {});

    REQUIRE(moved_level == "start.nw");
    REQUIRE(moved_x == 30.0f);
    REQUIRE(moved_y == 31.0f);
}

TEST_CASE("a class function can be called through its scope without joining", "[scripts]") {
    scripted_world world;

    world.add_class("mathfunctions", R"(
        public function Double(n) {
            return n * 2;
        }
    )");

    world.add_weapon("Caller", R"(
        public function onActionServerside() {
            serverr.doubled = mathfunctions::Double(21);
        }
    )");

    auto host = world.load();
    auto player = make_session();

    host.trigger_server(*player, "Caller", "go", {});

    REQUIRE(to_string(host.get_server_flags()->get("doubled")) == "42");
}

TEST_CASE("findLevel answers for levels and gmaps, and nothing else", "[scripts]") {
    scripted_world world;

    world.add_weapon("Asker", R"(
        public function onActionServerside() {
            serverr.real = (findLevel("testworld.nw") != NULL);
            serverr.fake = (findLevel("nowhere.nw") != NULL);
            serverr.named = findLevel("testworld.nw").name;
        }
    )");

    auto host = world.load();
    auto player = make_session();

    host.trigger_server(*player, "Asker", "go", {});

    REQUIRE(to_number(host.get_server_flags()->get("real")) == 1.0);
    REQUIRE(to_number(host.get_server_flags()->get("fake")) == 0.0);
    REQUIRE(to_string(host.get_server_flags()->get("named")) == "testworld.nw");
}

TEST_CASE("findNPC reaches a database npc's public functions", "[scripts]") {
    scripted_world world;

    world.add_npc("Tables", R"(
        function onCreated() {
            this.base = 100;
        }

        public function Required(level) {
            return this.base * level;
        }
    )");

    world.add_weapon("Asker", R"(
        public function onActionServerside() {
            serverr.needed = findNPC("Tables").Required(3);
            serverr.field = findNPC("Tables").base;
            serverr.missing = (findNPC("Nobody") == NULL);
        }
    )");

    auto host = world.load();
    auto player = make_session();

    host.tick(1.0);
    host.trigger_server(*player, "Asker", "go", {});

    REQUIRE(to_string(host.get_server_flags()->get("needed")) == "300");
    REQUIRE(to_string(host.get_server_flags()->get("field")) == "100");
    REQUIRE(to_number(host.get_server_flags()->get("missing")) == 1.0);
}

TEST_CASE("the world's start position is readable as a server flag", "[scripts]") {
    scripted_world world;

    world.add_weapon("Asker", R"(
        public function onActionServerside() {
            serverr.seen = server.start_level @ " " @ server.start_x @ "," @ server.start_y;
        }
    )");

    auto host = world.load();
    auto player = make_session();

    host.trigger_server(*player, "Asker", "go", {});

    REQUIRE(to_string(host.get_server_flags()->get("seen")) == "testworld.nw 30,30");
}

TEST_CASE("a client flag the player set is readable server-side", "[scripts]") {
    scripted_world world;

    world.add_weapon("Reader", R"(
        public function onActionServerside() {
            serverr.seen = player.client.nametype;
        }
    )");

    auto host = world.load();
    auto player = make_session();

    player->client_flags->put("nametype", value{"2"});

    host.trigger_server(*player, "Reader", "go", {});

    REQUIRE(to_string(host.get_server_flags()->get("seen")) == "2");
}

TEST_CASE("loadfolder lists the files a pattern matches", "[scripts]") {
    scripted_world world;

    create_directories(world.root / "guilds");
    ofstream(world.root / "guilds" / "reds.txt") << "name=reds\n";
    ofstream(world.root / "guilds" / "blues.txt") << "name=blues\n";
    ofstream(world.root / "guilds" / "notes.md") << "ignored\n";

    world.add_weapon("Lister", R"(
        public function onActionServerside() {
            temp.guilds.loadfolder("guilds/*.txt", false);
            serverr.found = temp.guilds.size() @ ":" @ temp.guilds[0] @ "," @ temp.guilds[1];

            temp.escape.loadfolder("../*", false);
            serverr.escaped = temp.escape.size();
        }
    )");

    auto host = world.load();
    auto player = make_session();

    host.trigger_server(*player, "Lister", "go", {});

    REQUIRE(to_string(host.get_server_flags()->get("found")) == "2:blues.txt,reds.txt");
    REQUIRE(to_number(host.get_server_flags()->get("escaped")) == 0.0);
}

TEST_CASE("findPlayer and allplayers reach everyone online", "[scripts]") {
    scripted_world world;

    world.add_weapon("Finder", R"(
        public function onActionServerside() {
            serverr.self = findPlayer("alice").account;
            serverr.missing = (findPlayer("nobody") == NULL);
            serverr.count = allplayers.size();
        }
    )");

    auto host = world.load();
    auto player = make_session();

    vector online{player.get()};
    host.on_list_players([&online] { return online; });

    host.trigger_server(*player, "Finder", "go", {});

    REQUIRE(to_string(host.get_server_flags()->get("self")) == "alice");
    REQUIRE(to_number(host.get_server_flags()->get("missing")) == 1.0);
    REQUIRE(to_number(host.get_server_flags()->get("count")) == 1.0);
}
