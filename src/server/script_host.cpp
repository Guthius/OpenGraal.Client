#include <server/script_host.hpp>

#include <server/server.hpp>
#include <shared/container.hpp>
#include <shared/text.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <print>
#include <sstream>

using namespace std;
using namespace og::gs2;
using namespace og::shared;

namespace og::server {
    void tracked_dictionary::put(const string_view name, gs2::value value) {
        basic_dictionary::put(name, std::move(value));
        changed_.emplace_back(name);
    }

    auto tracked_dictionary::erase(const string_view name) -> bool {
        const auto erased = basic_dictionary::erase(name);
        if (erased) {
            changed_.emplace_back(name);
        }

        return erased;
    }

    auto tracked_dictionary::take_changed() -> vector<string> {
        return std::exchange(changed_, {});
    }

    auto script_host::take_changed_server_flags() -> vector<string> {
        return server_flags_->take_changed();
    }

    namespace {
        constexpr double tick_interval = 0.1;

        auto read_file(const filesystem::path &path) -> string {
            ifstream ifs(path, ios::binary);
            ostringstream buffer;
            buffer << ifs.rdbuf();

            return buffer.str();
        }

        auto files_in(const filesystem::path &directory, const string_view extension) -> vector<filesystem::path> {
            vector<filesystem::path> paths;

            if (!is_directory(directory)) {
                return paths;
            }

            for (const auto &entry : filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file() && entry.path().extension() == extension) {
                    paths.push_back(entry.path());
                }
            }

            ranges::sort(paths);

            return paths;
        }

        auto text_of(const values &args, const size_t index) -> string {
            return index < args.size() ? to_string(args[index]) : string{};
        }

        struct class_function_call : callable {
            gs2::script_ptr classes;
            dictionary_ptr self;
            string function;

            class_function_call(gs2::script_ptr classes, dictionary_ptr self, string function)
                : classes(std::move(classes)), self(std::move(self)), function(std::move(function)) {
            }

            auto invoke(const values &args) -> expected_value override {
                return classes->call_public(function, self, args);
            }
        };

        class npc_dictionary : public basic_dictionary {
          public:
            explicit npc_dictionary(std::shared_ptr<npc> target) : target_(std::move(target)) {
            }

            auto contains(const string_view name) -> bool override {
                return target_->self->contains(name) || has_function(name);
            }

            auto get(const string_view name) -> value override {
                if (target_->self->contains(name) || !has_function(name)) {
                    return target_->self->get(name);
                }

                return value{make_shared<class_function_call>(target_->script, target_->self, string(name))};
            }

            void put(const string_view name, value val) override {
                target_->self->put(name, std::move(val));
            }

          private:
            [[nodiscard]] auto has_function(const string_view name) const -> bool {
                return target_->script && target_->script->has_function(name);
            }

            std::shared_ptr<npc> target_;
        };

    }

    script_host::script_host(world &game_world) : world_(game_world) {
        bind_types();
    }

    void script_host::report(const error &failure) const {
        println(cerr, "[script] {}{}{}: {} (line {})",
            failure.source_name,
            failure.function_name.empty() ? "" : ".",
            failure.function_name,
            failure.message,
            failure.position.line);
    }

    auto script_host::make_script(const string &name, const string &source) -> script_ptr {
        istringstream is(source);

        auto script = load_script(env_, is);
        if (!script) {
            auto failure = script.error();
            failure.source_name = name;

            report(failure);

            return nullptr;
        }

        (*script)->set_name(name);

        return *script;
    }

    void script_host::load() {
        env_.set_class_resolver([this](const string_view name) -> optional<string> {
            const auto path = world_.root() / "scripts" / (string(name) + ".txt");
            if (!exists(path)) {
                return nullopt;
            }

            return split_script(read_file(path)).server;
        });

        env_.set_file_resolver([this](const string_view name) -> optional<string> {
            if (name.empty() || name.contains("..")) {
                return nullopt;
            }

            const auto path = world_.root() / name;
            if (!exists(path) || !is_regular_file(path)) {
                return nullopt;
            }

            return read_file(path);
        });

        env_.set_folder_resolver([this](const string_view pattern, const bool recursive) -> vector<string> {
            vector<string> found;

            const auto star = pattern.find('*');
            if (pattern.contains("..") || star == string_view::npos) {
                return found;
            }

            const auto prefix = pattern.substr(0, star);
            const auto suffix = pattern.substr(star + 1);
            const auto directory = world_.root() / prefix;

            error_code failed;
            if (!is_directory(directory, failed)) {
                return found;
            }

            const auto collect = [&](const filesystem::directory_entry &entry) {
                if (!entry.is_regular_file()) {
                    return;
                }

                auto name = filesystem::relative(entry.path(), world_.root() / prefix, failed).generic_string();
                if (!failed && (suffix.empty() || name.ends_with(suffix))) {
                    found.push_back(std::move(name));
                }
            };

            if (recursive) {
                for (const auto &entry : filesystem::recursive_directory_iterator(directory, failed)) {
                    collect(entry);
                }
            } else {
                for (const auto &entry : filesystem::directory_iterator(directory, failed)) {
                    collect(entry);
                }
            }

            ranges::sort(found);

            return found;
        });

        env_.set_file_writer([this](const string_view name, const string_view contents) -> bool {
            if (name.empty() || name.contains("..")) {
                return false;
            }

            const auto path = world_.root() / name;

            error_code failed;
            create_directories(path.parent_path(), failed);

            ofstream os(path, ios::out | ios::trunc | ios::binary);
            if (!os.is_open()) {
                return false;
            }

            os << contents;

            return os.good();
        });

        env_.put("server", value{server_flags_});
        env_.put("serverr", value{server_flags_});

        for (const auto &[key, entries] : world_.flags().values()) {
            if (!entries.empty()) {
                server_flags_->put(key, value{entries.front()});
            }
        }

        server_flags_->put("start_level", value{world_.start().level});
        server_flags_->put("start_x", value{static_cast<double>(world_.start().x)});
        server_flags_->put("start_y", value{static_cast<double>(world_.start().y)});

        for (const auto &path : files_in(world_.root() / "weapons", ".txt")) {
            const auto container = load_weapon(path);
            if (!container) {
                println(cerr, "[script] {}", container.error());

                continue;
            }

            auto halves = split_script(container->script);

            weapons_.push_back(weapon{
                .name = container->name,
                .image = container->image,
                .client_script = std::move(halves.client),
                .script = make_script(container->name, halves.server),
            });
        }

        for (const auto &path : files_in(world_.root() / "npcs", ".txt")) {
            const auto container = load_npc(path);
            if (!container) {
                println(cerr, "[script] {}", container.error());

                continue;
            }

            auto halves = split_script(container->script);

            auto entry = make_shared<npc>();
            entry->id = next_npc_id_++;
            entry->name = container->name;
            entry->level = container->attribute("LEVEL", container->attribute("STARTLEVEL"));
            entry->script = make_script(container->name, halves.server);
            entry->state = net::npc_state{
                .id = entry->id,
                .x = container->attribute_float("X", container->attribute_float("STARTX")),
                .y = container->attribute_float("Y", container->attribute_float("STARTY")),
                .image = container->image,
                .nickname = container->attribute("NICK"),
            };

            npcs_.push_back(entry);
        }

        println("Loaded {} weapon(s) and {} database npc(s)", weapons_.size(), npcs_.size());
    }

    void script_host::ensure_level(const string &level_name) {
        if (level_name.empty() || ranges::contains(instantiated_levels_, level_name)) {
            return;
        }

        instantiated_levels_.push_back(level_name);

        const auto level = world_.find_level(level_name);
        if (!level) {
            return;
        }

        for (const auto &source : level->npcs) {
            if (source.script.find_first_not_of(" \t\r\n") == string::npos) {
                continue;
            }

            auto halves = split_script(source.script);

            auto entry = make_shared<npc>();
            entry->id = next_npc_id_++;
            entry->level = level_name;
            entry->name = format("{}#{}", level_name, entry->id);
            entry->script = make_script(entry->name, halves.server);
            entry->state = net::npc_state{
                .id = entry->id,
                .x = source.x,
                .y = source.y,
                .image = source.image,
            };

            npcs_.push_back(entry);
        }
    }

    auto script_host::reset_level(const string &level_name) -> vector<uint32_t> {
        vector<uint32_t> removed;

        erase_if(npcs_, [&](const shared_ptr<npc> &entry) {
            if (!iequals(entry->level, level_name)) {
                return false;
            }

            removed.push_back(entry->id);

            return true;
        });

        erase_if(instantiated_levels_, [&](const string &name) { return iequals(name, level_name); });

        return removed;
    }

    auto script_host::npcs_in(const string &level) const -> vector<shared_ptr<npc>> {
        vector<shared_ptr<npc>> found;

        for (const auto &entry : npcs_) {
            if (entry->level == level) {
                found.push_back(entry);
            }
        }

        return found;
    }

    auto script_host::find_weapon(const string_view name) const -> const weapon * {
        const auto match = ranges::find_if(weapons_, [name](const weapon &entry) {
            return iequals(entry.name, name);
        });

        return match == weapons_.end() ? nullptr : &*match;
    }

    void script_host::item_taken(session &player, const string &item_name) {
        for (const auto &entry : weapons_) {
            if (!entry.script || !entry.script->has_function("onPlayerTakesItem")) {
                continue;
            }

            auto self = make_shared<basic_dictionary>();

            env_.put("player", value{make_player_object(player)});
            env_.put("client", value{player.client_flags});
            env_.put("clientr", value{player.clientr_flags});
            env_.put("allplayers", value{list_players()});

            const auto result = entry.script->call_with("onPlayerTakesItem", self, nullptr, {value{item_name}});

            env_.erase("player");
            env_.erase("client");
            env_.erase("clientr");

            if (!result) {
                report(result.error());
            }
        }
    }

    void script_host::bind_types() {
        env_.register_function("addweapon", [this](environment &, const values &args) -> expected_value {
            const auto name = text_of(args, 0);
            if (active_player_ == nullptr || name.empty()) {
                return value{0.0};
            }

            const auto owned = ranges::contains(active_player_->weapons, name);
            if (!owned) {
                active_player_->weapons.push_back(name);
            }

            if (weapon_sink_) {
                weapon_sink_(*active_player_, name, true);
            }

            return value{owned ? 0.0 : 1.0};
        });

        env_.register_function("removeweapon", [this](environment &, const values &args) -> expected_value {
            const auto name = text_of(args, 0);
            if (active_player_ == nullptr) {
                return value{0.0};
            }

            if (erase(active_player_->weapons, name) == 0) {
                return value{0.0};
            }

            if (weapon_sink_) {
                weapon_sink_(*active_player_, name, false);
            }

            return value{1.0};
        });

        env_.register_function("findnpc", [this](environment &, const values &args) -> expected_value {
            const auto name = text_of(args, 0);

            for (const auto &entry : npcs_) {
                if (iequals(entry->name, name)) {
                    return value{make_shared<npc_dictionary>(entry)};
                }
            }

            return value{};
        });

        env_.register_function("findplayer", [this](environment &, const values &args) -> expected_value {
            const auto account = text_of(args, 0);
            if (account.empty() || !player_source_) {
                return value{};
            }

            for (auto *online : player_source_()) {
                if (iequals(online->profile.name, account)) {
                    return value{make_player_object(*online)};
                }
            }

            return value{};
        });

        env_.register_function("findlevel", [this](environment &, const values &args) -> expected_value {
            const auto name = text_of(args, 0);
            if (name.empty()) {
                return value{};
            }

            const auto known = world_.find_level(name) != nullptr ||
                               ranges::any_of(world_.get_gmaps(), [&name](const shared::gmap_data &map) {
                                   return iequals(map.name, name);
                               });

            if (!known) {
                return value{};
            }

            auto level = make_shared<basic_dictionary>();
            level->put("name", value{name});

            return value{level};
        });

        env_.register_function("setlevel2", [this](environment &, const values &args) -> expected_value {
            if (level_sink_ && active_player_ != nullptr && !args.empty()) {
                level_sink_(*active_player_,
                    to_string(args[0]),
                    args.size() > 1 ? static_cast<float>(to_number(args[1])) : 0.0f,
                    args.size() > 2 ? static_cast<float>(to_number(args[2])) : 0.0f);
            }

            return {};
        });

        for (const auto *name : {"sendtext", "requesttext"}) {
            env_.register_function(name, [](environment &, const values &) -> expected_value {
                return {};
            });
        }

        env_.register_function("echo", [](environment &, const values &args) -> expected_value {
            println("[script] {}", text_of(args, 0));

            return {};
        });

        env_.register_function("setTimer", [this](environment &, const values &args) -> expected_value {
            if (active_ != nullptr) {
                active_->timer = args.empty() ? 0.0 : to_number(args.front());
                active_->self->put("timeout", value{active_->timer});
            }

            return {};
        });

        env_.register_function("triggeraction", [this](environment &, const values &args) -> expected_value {
            const auto action = args.size() > 2 ? to_string(args[2]) : string{};

            if (folded_equal{}(action, "clientside")) {
                if (!trigger_sink_ || active_player_ == nullptr) {
                    return {};
                }

                values extra;
                for (size_t i = 5; i < args.size(); ++i) {
                    extra.push_back(args[i]);
                }

                trigger_sink_(active_player_->connection,
                    args.size() > 3 ? to_string(args[3]) : string{},
                    args.size() > 4 ? to_string(args[4]) : string{},
                    extra);

                return {};
            }

            if (active_player_ == nullptr) {
                return {};
            }

            values extra;
            for (size_t i = 3; i < args.size(); ++i) {
                extra.push_back(args[i]);
            }

            trigger_action(*active_player_, active_player_->level,
                static_cast<float>(args.empty() ? 0.0 : to_number(args[0])),
                static_cast<float>(args.size() > 1 ? to_number(args[1]) : 0.0),
                action, extra);

            return {};
        });

        env_.register_function("putitem", [this](environment &, const values &args) -> expected_value {
            if (item_sink_) {
                item_sink_(text_of(args, 0),
                    static_cast<float>(args.size() > 1 ? to_number(args[1]) : 0.0),
                    static_cast<float>(args.size() > 2 ? to_number(args[2]) : 0.0),
                    text_of(args, 3),
                    text_of(args, 4));
            }

            return {};
        });
    }

    namespace {
        struct player_flag_call : callable {
            bool *flag;

            explicit player_flag_call(bool *flag) : flag(flag) {
            }

            auto invoke(const values &args) -> expected_value override {
                if (flag != nullptr) {
                    *flag = args.empty() || to_bool(args.front());
                }

                return {};
            }
        };

        struct server_warp_call : callable {
            script_host::warp_sink sink;
            net::connection_ptr target;
            string account;

            server_warp_call(script_host::warp_sink sink, net::connection_ptr target, string account)
                : sink(std::move(sink)), target(std::move(target)), account(std::move(account)) {
            }

            auto invoke(const values &args) -> expected_value override {
                if (!sink) {
                    return {};
                }

                const auto host = args.empty() ? string{} : to_string(args[0]);
                const auto port = args.size() > 1 ? static_cast<uint16_t>(to_number(args[1])) : 0;

                sink(target, host, port, account);

                return {};
            }
        };

        auto to_values(const vector<string> &names) -> values {
            values result;
            result.reserve(names.size());

            for (const auto &name : names) {
                result.emplace_back(name);
            }

            return result;
        }

        auto empty_script(environment &env) -> gs2::script_ptr {
            istringstream source;

            auto script = load_script(env, source);

            return script ? *script : nullptr;
        }

        class player_dictionary : public basic_dictionary {
          public:
            explicit player_dictionary(session &owner) : owner_(&owner) {
            }

            auto contains(const string_view name) -> bool override {
                return basic_dictionary::contains(name) || in_classes(name);
            }

            auto get(const string_view name) -> value override {
                if (basic_dictionary::contains(name) || !in_classes(name)) {
                    return basic_dictionary::get(name);
                }

                return value{make_shared<class_function_call>(owner_->classes, owner_->script_object, string(name))};
            }

          private:
            [[nodiscard]] auto in_classes(const string_view name) const -> bool {
                return owner_->classes && owner_->classes->has_function(name);
            }

            session *owner_;
        };

        struct set_level_call : callable {
            script_host::level_sink sink;
            session *owner;

            set_level_call(script_host::level_sink sink, session &owner) : sink(std::move(sink)), owner(&owner) {
            }

            auto invoke(const values &args) -> expected_value override {
                if (!sink || args.empty()) {
                    return {};
                }

                sink(*owner,
                    to_string(args[0]),
                    args.size() > 1 ? static_cast<float>(to_number(args[1])) : 0.0f,
                    args.size() > 2 ? static_cast<float>(to_number(args[2])) : 0.0f);

                return {};
            }
        };

        struct player_join_call : callable {
            environment *env;
            session *owner;

            player_join_call(environment &env, session &owner) : env(&env), owner(&owner) {
            }

            auto invoke(const values &args) -> expected_value override {
                if (args.empty() || !owner->classes) {
                    return value{0.0};
                }

                const auto name = to_string(args.front());

                auto joined = env->find_class(name);
                if (!joined) {
                    return unexpected(error{
                        .source = error_source::interpreter,
                        .kind = error_kind::runtime_error,
                        .message = format("unknown class '{}'", name),
                    });
                }

                if (!owner->classes->join(joined)) {
                    return value{0.0};
                }

                owner->joined_classes.push_back(name);

                return value{1.0};
            }
        };

        struct trigger_client_call : callable {
            script_host::trigger_sink sink;
            net::connection_ptr target;

            trigger_client_call(script_host::trigger_sink sink, net::connection_ptr target)
                : sink(std::move(sink)), target(std::move(target)) {
            }

            auto invoke(const values &args) -> expected_value override {
                if (!sink) {
                    return {};
                }

                const auto name = args.size() > 1 ? to_string(args[1]) : string{};
                const auto action = args.size() > 2 ? to_string(args[2]) : string{};

                values extra;
                for (size_t i = 3; i < args.size(); ++i) {
                    extra.push_back(args[i]);
                }

                sink(target, name, action, extra);

                return {};
            }
        };
    }

    auto script_host::list_players() -> array_ptr {
        auto everyone = make_shared<gs2::array>();

        if (player_source_) {
            for (auto *online : player_source_()) {
                everyone->elements.emplace_back(make_player_object(*online));
            }
        }

        return everyone;
    }

    auto script_host::make_player_object(session &player) -> dictionary_ptr {
        if (!player.script_object) {
            player.attr = make_shared<gs2::array>(values(net::player_attr_count));
            player.classes = empty_script(env_);
            player.script_object = make_shared<player_dictionary>(player);

            auto &created = *player.script_object;

            created.put("triggerclient", value{make_shared<trigger_client_call>(trigger_sink_, player.connection)});
            created.put("setvisible", value{make_shared<player_flag_call>(&player.state.visible)});
            created.put("serverwarp", value{make_shared<server_warp_call>(warp_sink_, player.connection, player.profile.name)});
            created.put("join", value{make_shared<player_join_call>(env_, player)});
            created.put("setlevel2", value{make_shared<set_level_call>(level_sink_, player)});
            created.put("attr", value{player.attr});
            created.put("client", value{player.client_flags});
            created.put("clientr", value{player.clientr_flags});
            created.put("z", value{0.0});
            created.put("freezetime", value{-1.0});
        }

        auto &object = *player.script_object;

        object.put("visible", value{player.state.visible ? 1.0 : 0.0});
        object.put("account", value{player.profile.name});
        object.put("nick", value{player.state.nickname});
        object.put("x", value{static_cast<double>(player.state.x)});
        object.put("y", value{static_cast<double>(player.state.y)});
        object.put("dir", value{static_cast<double>(player.state.direction)});
        object.put("ani", value{player.state.animation});
        object.put("headimg", value{player.state.head});
        object.put("bodyimg", value{player.state.body});

        auto level = make_shared<basic_dictionary>();

        level->put("name", value{player.level});

        object.put("level", value{level});
        object.put("chat", value{player.last_chat});
        object.put("id", value{static_cast<double>(player.state.id)});
        object.put("joinedclasses", value{make_shared<gs2::array>(to_values(player.joined_classes))});

        for (size_t i = 0; i < player.attr->elements.size() && i < player.state.attr.size(); ++i) {
            player.attr->elements[i] = value{player.state.attr[i]};
        }

        return player.script_object;
    }

    void script_host::write_back_player(session &player) {
        if (!player.script_object) {
            return;
        }

        auto &object = *player.script_object;
        auto changed = false;

        const auto text = [&](const string_view name, string &field) {
            if (auto written = to_string(object.get(name)); written != field) {
                field = std::move(written);
                changed = true;
            }
        };

        const auto number = [&](const string_view name, auto &field) {
            if (const auto written = static_cast<decay_t<decltype(field)>>(to_number(object.get(name))); written != field) {
                field = written;
                changed = true;
            }
        };

        text("nick", player.state.nickname);
        text("ani", player.state.animation);
        text("headimg", player.state.head);
        text("bodyimg", player.state.body);
        number("x", player.state.x);
        number("y", player.state.y);
        number("dir", player.state.direction);

        for (size_t i = 0; i < player.attr->elements.size() && i < player.state.attr.size(); ++i) {
            if (auto written = to_string(player.attr->elements[i]); written != player.state.attr[i]) {
                player.state.attr[i] = std::move(written);
                changed = true;
            }
        }

        player.state_changed = player.state_changed || changed;
    }

    void script_host::dispatch(const shared_ptr<npc> &target, const string_view event, const values &args, session *player) {
        if (!target->script || !target->script->has_function(event)) {
            return;
        }

        auto self = target->self;

        self->put("id", value{static_cast<double>(target->id)});
        self->put("x", value{static_cast<double>(target->state.x)});
        self->put("y", value{static_cast<double>(target->state.y)});
        self->put("level", value{target->level});
        self->put("chat", value{target->state.chat});
        self->put("nick", value{target->state.nickname});
        self->put("image", value{target->state.image});
        self->put("timeout", value{target->timer});

        if (player != nullptr) {
            env_.put("player", value{make_player_object(*player)});
            env_.put("client", value{player->client_flags});
            env_.put("clientr", value{player->clientr_flags});
            env_.put("allplayers", value{list_players()});
        }

        auto level_object = make_shared<basic_dictionary>();
        level_object->put("name", value{target->level});
        env_.put("level", value{level_object});

        active_ = target.get();
        active_player_ = player;

        const auto result = target->script->call_with(event, self, target->locals, args);

        active_ = nullptr;
        active_player_ = nullptr;

        if (player != nullptr) {
            write_back_player(*player);
        }

        env_.erase("player");
        env_.erase("client");
        env_.erase("clientr");
        env_.erase("level");

        if (!result) {
            report(result.error());

            return;
        }

        const auto pull_number = [&self](const string_view key, float &destination) {
            if (self->contains(key)) {
                destination = static_cast<float>(to_number(self->get(key)));
            }
        };

        const auto pull_text = [&self](const string_view key, string &destination) {
            if (self->contains(key)) {
                destination = to_string(self->get(key));
            }
        };

        const auto previous = target->state;

        pull_number("x", target->state.x);
        pull_number("y", target->state.y);
        pull_text("image", target->state.image);
        pull_text("nick", target->state.nickname);
        pull_text("chat", target->state.chat);

        target->timer = to_number(self->get("timeout"));

        if (target->state.chat != previous.chat && !target->state.chat.empty() && chat_sink_) {
            chat_sink_(*target, target->state.chat);
        }

        if (target->state.x != previous.x ||
            target->state.y != previous.y ||
            target->state.image != previous.image ||
            target->state.nickname != previous.nickname ||
            target->state.chat != previous.chat) {
            target->dirty = true;
        }
    }

    void script_host::tick(const double dt) {
        accumulator_ += dt;

        if (accumulator_ < tick_interval) {
            return;
        }

        const auto elapsed = accumulator_;
        accumulator_ = 0.0;

        for (const auto &entry : npcs_) {
            if (!entry->created) {
                entry->created = true;

                dispatch(entry, "onCreated", {}, nullptr);
            }

            if (entry->timer <= 0.0) {
                continue;
            }

            entry->timer -= elapsed;

            if (entry->timer <= 0.0) {
                entry->timer = 0.0;

                dispatch(entry, "onTimeout", {}, nullptr);
            }
        }
    }

    void script_host::player_online(session &player) {
        for (const auto &entry : npcs_) {
            if (entry->script && entry->script->has_function("onActionPlayerOnline")) {
                dispatch(entry, "onActionPlayerOnline", {}, &player);
            }
        }
    }

    void script_host::player_logged_in(session &player) {
        const auto self = value{make_player_object(player)};

        for (const auto &entry : npcs_) {
            if (entry->script && entry->script->has_function("onPlayerLogin")) {
                dispatch(entry, "onPlayerLogin", {self}, &player);
            }
        }
    }

    void script_host::player_offline(session &player) {
        const auto self = value{make_player_object(player)};

        for (const auto &entry : npcs_) {
            if (entry->script && entry->script->has_function("onPlayerLogout")) {
                dispatch(entry, "onPlayerLogout", {self}, &player);
            }
        }
    }

    void script_host::player_entered(session &player) {
        ensure_level(player.level);

        for (const auto &entry : npcs_in(player.level)) {
            dispatch(entry, "onPlayerEnters", {}, &player);
        }
    }

    void script_host::player_left(session &player) {
        for (const auto &entry : npcs_in(player.level)) {
            dispatch(entry, "onPlayerLeaves", {}, &player);
        }
    }

    void script_host::player_chatted(session &player, const string &text) {
        for (const auto &entry : npcs_in(player.level)) {
            dispatch(entry, "onPlayerChats", {value{text}}, &player);
        }
    }

    void script_host::trigger_action(session &player, const string &level_name, const float x, const float y, const string &action, const values &args) {
        constexpr float npc_size = 2.0f;

        const auto event = "onAction" + action;

        for (const auto &entry : npcs_in(level_name)) {
            if (x < entry->state.x || x > entry->state.x + npc_size ||
                y < entry->state.y || y > entry->state.y + npc_size) {
                continue;
            }

            dispatch(entry, event, args, &player);
        }
    }

    void script_host::trigger_server(session &player, const string &weapon_name, const string &action, const values &args) {
        const auto *entry = find_weapon(weapon_name);
        if (entry == nullptr || !entry->script) {
            return;
        }

        auto self = make_shared<basic_dictionary>();
        self->put("action", value{action});

        auto arguments = values{value{action}};
        arguments.insert(arguments.end(), args.begin(), args.end());

        env_.put("player", value{make_player_object(player)});
        env_.put("client", value{player.client_flags});
        env_.put("clientr", value{player.clientr_flags});
        env_.put("allplayers", value{list_players()});

        active_player_ = &player;

        const auto result = entry->script->call("onActionServerside", self, arguments);

        active_player_ = nullptr;

        write_back_player(player);

        env_.erase("player");
        env_.erase("client");
        env_.erase("clientr");

        if (!result) {
            report(result.error());
        }
    }
}
