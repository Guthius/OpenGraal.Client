#include "game.hpp"

#include "animation_manager.hpp"
#include "chat_bar.hpp"
#include "client/remote_player.hpp"
#include "client_prefs.hpp"
#include "dev_input.hpp"
#include "editor/editor_screen.hpp"
#include "file_manager.hpp"
#include "font_manager.hpp"
#include "gui/gui_control.hpp"
#include "key_codes.hpp"
#include "level_manager.hpp"
#include "login_screen.hpp"
#include "network.hpp"
#include "player.hpp"
#include "screenshot.hpp"
#include "script_host.hpp"
#include "sign.hpp"
#include "sound_manager.hpp"
#include "texture_manager.hpp"
#include "thrown_item.hpp"
#include "tileset_manager.hpp"

#include <shared/terrain.hpp>
#include <shared/text.hpp>

#include <format>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <sstream>

#define SIGN_WIDTH  382
#define SIGN_HEIGHT 142

namespace {
    struct level_info {
        level_info(const std::shared_ptr<level> &level, tileset *tileset) : level(level), tileset(tileset) {
        }

        std::shared_ptr<level> level;
        tileset *tileset;
    };

    std::unique_ptr<sign> current_sign;

    enum class game_state : uint8_t {
        login,
        playing,
        editor
    };

    constexpr Color login_sky_top{1, 0, 40, 255};
    constexpr Color login_sky_bottom{36, 0, 128, 255};

    game_state state = game_state::login;
    std::string pending_account;
    std::string pending_password;
    std::string pending_nickname;
    bool pending_save_password = false;
    level_info *current_level_info;
    player *local_player;
    Texture2D state_texture{};
    Texture2D sprites_texture{};
    Font font_24{};
    Font font_18{};
    Font font_pixel_12{};
    std::vector<thrown_item> active_thrown_items{};
    std::vector<leap_effect> active_leaps{};
    std::shared_ptr<gui_control> gui;

    struct gmap_window {
        bool active = false;
        int cell_x = 0;
        int cell_y = 0;
        std::array<std::string, 9> names{};
        std::array<std::shared_ptr<level>, 9> levels{};
    };

    gmap_window current_gmap;
    network::gmap_terrain current_terrain;
    std::string pending_level;

    void fill_generated_tiles(const std::shared_ptr<level> &target, const int cell_x, const int cell_y) {
        if (!target || target->has_tiles() || current_terrain.heights.empty()) {
            return;
        }

        og::shared::gmap_data map;
        map.width = current_terrain.width;
        map.height = current_terrain.height;
        map.height_map = current_terrain.heights;
        map.level_height = current_terrain.level_height;
        map.level_chaos = current_terrain.level_chaos;
        map.random_seeds = {current_terrain.seed};

        target->set_board(generate_level_tiles(map, {.x = cell_x, .y = cell_y}));
    }

    void update_thrown_items(const float dt) {
        for (auto &item : active_thrown_items) {
            item.update(dt);

            if (!item.is_alive()) {
                spawn_leaps(item.get_leap_type(), item.get_position());
            }
        }

        std::erase_if(active_thrown_items, [](const thrown_item &item) {
            return !item.is_alive();
        });
    }

    void update_leaps(const float dt) {
        for (auto &leap : active_leaps) {
            leap.update(dt);
        }

        std::erase_if(active_leaps, [](const leap_effect &item) {
            return !item.is_alive();
        });
    }

    void update() {
        const auto dt = GetFrameTime();

        gui->set_size({static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())});
        gui->update(dt);

        if (state == game_state::login) {
            get_network().update(dt);
            get_login_screen().update(dt);

            return;
        }

        if (state == game_state::editor) {
            get_network().update(dt);
            get_editor_screen().update(dt);

            return;
        }

        if (dev_input::key_pressed(KEY_F11) && get_network().is_connected()) {
            get_network().send_edit_request("");
        }

        update_thrown_items(dt);
        update_leaps(dt);

        if (current_sign->is_open()) {
            current_sign->update();

            return;
        }

        const auto chat_open = get_chat_bar().update();

        if (get_script_host().is_default_movement_enabled() && !chat_open) {
            local_player->update(dt);
        } else {
            local_player->update_animation(dt);
        }

        if (!chat_open) {
            for (const auto keycode : key_codes::pressed_this_frame()) {
                get_script_host().key_pressed(keycode);
            }
        }

        auto &network = get_network();
        network.update(dt);

        get_script_host().update(dt);

        if (network.is_connected()) {
            const auto [x, y] = local_player->get_position();

            network.send_move(
                x,
                y,
                static_cast<int>(local_player->get_direction()),
                local_player->get_animation());
        }
    }

    void draw_diagnostics() {
        const auto &network = get_network();
        const auto *const str =
            network.is_enabled()
                ? TextFormat("FPS: %d | %s | %d players",
                      GetFPS(),
                      network.get_status().c_str(),
                      static_cast<int>(network.get_players().size()) + 1)
                : TextFormat("FPS: %d | offline", GetFPS());

        DrawTextEx(font_pixel_12, str, {6, 6}, 12, 1, BLACK);
        DrawTextEx(font_pixel_12, str, {5, 5}, 12, 1, WHITE);
    }

    void draw_thrown_items(const std::vector<thrown_item> &thrown_items) {
        for (const auto &thrown_item : thrown_items) {
            thrown_item.draw();
        }
    }

    void draw_leaps(const std::vector<leap_effect> &leaps) {
        for (const auto &leap : leaps) {
            leap.draw();
        }
    }

    void draw_items() {
        for (const auto &[id, item] : get_network().get_items()) {
            const auto position = Vector2{item.x * tile_size, item.y * tile_size};

            if (!item.image.empty()) {
                if (const auto texture = load_texture(item.image); IsTextureValid(texture)) {
                    DrawTextureV(texture, position, WHITE);

                    continue;
                }
            }

            DrawCircleV({position.x + 8, position.y + 8}, 5, GOLD);
            DrawCircleLinesV({position.x + 8, position.y + 8}, 5, ORANGE);
        }
    }

    void draw_npcs() {
        const auto font = load_font("LiberationSans-Regular.ttf", 12);

        for (const auto &[id, npc] : get_network().get_npcs()) {
            const auto position = Vector2{npc.x * tile_size, npc.y * tile_size};

            if (!npc.image.empty()) {
                if (const auto texture = load_texture(npc.image); IsTextureValid(texture)) {
                    DrawTextureV(texture, position, WHITE);
                }
            }

            const auto label = npc.chat.empty() ? npc.nickname : npc.chat;
            if (label.empty()) {
                continue;
            }

            const auto width = MeasureTextEx(font, label.c_str(), 12, 1).x;
            const auto anchor = Vector2{position.x + tile_size - (width / 2), position.y - 14};

            DrawRectangle(
                static_cast<int>(anchor.x) - 3,
                static_cast<int>(anchor.y) - 2,
                static_cast<int>(width) + 6,
                16,
                Fade(BLACK, 0.55f));

            DrawTextEx(font, label.c_str(), anchor, 12, 1, npc.chat.empty() ? SKYBLUE : WHITE);
        }
    }

    void draw() {
        constexpr auto camera_offset = Vector2(16, 16);
        const auto screen_size = Vector2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
        const auto screen_half_size = screen_size / 2.0f;
        const auto [cx, cy] = screen_half_size - local_player->get_position() - camera_offset;

        rlSetTexture(current_level_info->tileset->get_texture().id);

        rlPushMatrix();
        rlTranslatef(cx, cy, 0);

        if (current_gmap.active) {
            for (int i = 0; i < 9; ++i) {
                const auto &neighbour = current_gmap.levels[i];
                if (!neighbour || i == 4) {
                    continue;
                }

                const auto offset_x = static_cast<float>((i % 3) - 1) * level_pixel_size;
                const auto offset_y = static_cast<float>((i / 3) - 1) * level_pixel_size;

                rlPushMatrix();
                rlTranslatef(offset_x, offset_y, 0);
                neighbour->draw(current_level_info->tileset);
                rlPopMatrix();
            }
        }

        if (current_level_info->level) {
            current_level_info->level->draw(current_level_info->tileset);
        }

        local_player->draw();

        for (const auto &[id, player] : get_network().get_players()) {
            player.draw();
        }

        draw_items();
        draw_npcs();

        draw_thrown_items(active_thrown_items);
        draw_leaps(active_leaps);

        rlPopMatrix();
    }

    void init() {
        build_file_table("levels");

        constexpr Vector2 pos{512, 512};

        local_player = new player();
        local_player->set_position(pos);

        current_level_info = new level_info(
            load_level("onlinestartlocal.nw"),
            load_tileset("pics1.png"));

        state_texture = load_texture("state.png");

        font_24 = LoadFontEx("levels/fonts/LiberationSans-Bold.ttf", 24, nullptr, 250);
        font_18 = LoadFontEx("levels/fonts/LiberationSans-Regular.ttf", 18, nullptr, 250);
        font_pixel_12 = LoadFontEx("levels/fonts/Kenney Pixel.ttf", 12, nullptr, 0);

        current_sign = std::make_unique<sign>();

        sprites_texture = load_texture("sprites.png");

        gui = std::make_shared<gui_control>();

        get_script_host().set_gui_root(gui);

        get_chat_bar().attach(gui);
        get_login_screen().attach(gui);
        get_editor_screen().attach(gui);
        get_script_host().bind_builtin_objects();

        get_chat_bar().on_submit([](const std::string &text) {
            get_script_host().player_chatted(text);

            if (get_network().is_connected()) {
                get_network().send_chat(text);
            } else {
                show_sign(text);
            }
        });
    }
}

void on_level_downloaded(const std::string &name, const std::string &data);
void on_level_reloaded(const std::string &name, const std::string &data);

namespace {
    auto connect_for(const network_settings *config, const std::string &account, const std::string &password, const std::string &nickname) -> bool {
        auto target = config != nullptr ? *config : network_settings{};

        target.account = account;
        target.password = password;
        target.nickname = nickname;
        target.token.clear();

        return get_network().connect(target);
    }

    void begin_login(const network_settings *config, const std::string &account, const std::string &password, const std::string &nickname, const bool save) {
        pending_account = account;
        pending_password = password;
        pending_nickname = nickname;
        pending_save_password = save;

        if (!connect_for(config, account, password, nickname)) {
            get_login_screen().set_status(get_network().get_status());
            get_login_screen().set_busy(false);

            return;
        }

        get_network().send_login();
    }

    void begin_create(const network_settings *config, const std::string &account, const std::string &password) {
        pending_account = account;
        pending_password = password;

        if (!get_network().is_connected() && !connect_for(config, account, password, pending_nickname)) {
            get_login_screen().set_status(get_network().get_status());

            return;
        }

        get_network().send_create_account(account, password);
    }
}

void run_game(const network_settings *network_config, const screenshot_settings *screenshot) {
    init();

    {
        auto &network = get_network();

        network.on_level([](const std::string &level, const float x, const float y) {
            clear_gmap_window();
            change_level(level);

            const auto position = Vector2{x * tile_size, y * tile_size};

            local_player->set_position(position);

            if (pending_level.empty()) {
                local_player->try_move_from_wall(position);

                get_script_host().player_entered_level();
            }
        });

        network.on_chat([](const std::string &nickname, const std::string &text) {
            TraceLog(LOG_INFO, "CHAT: %s: %s", nickname.c_str(), text.c_str());
        });

        network.on_warp([] {
            get_script_host().clear();

            clear_gmap_window();
        });

        network.on_file([](const std::string &name, const std::string &data) {
            if (name.ends_with(".nw") || name.ends_with(".graal")) {
                on_level_downloaded(name, data);

                return;
            }

            if (store_file(name, data)) {
                forget_texture(name);
                forget_animation(name);

                refresh_tileset();
            }
        });

        network.on_gmap([](const std::string &, const int cell_x, const int cell_y, const std::array<std::string, 9> &neighbours, const network::gmap_terrain &terrain) {
            set_gmap_window(cell_x, cell_y, neighbours, terrain);
        });

        network.on_level_reload([](const std::string &name, const std::string &data) {
            on_level_reloaded(name, data);
        });

        network.on_edit_response([](const std::string &level, const og::net::edit_status status, const std::string &detail) {
            if (status == og::net::edit_status::ok) {
                get_network().send_edit_fetch_level(level);

                return;
            }

            const auto reason = detail.empty()
                                    ? std::string(og::net::describe(status))
                                    : std::format("{} {}", og::net::describe(status), detail);

            if (state == game_state::editor) {
                get_editor_screen().set_status(std::format("Cannot edit {}: {}", level, reason));
            } else {
                show_sign(std::format("Cannot edit {}: {}", level, reason));
            }
        });

        network.on_edit_content([](const std::string &name, const og::net::edit_status status, const std::string &data) {
            if (status != og::net::edit_status::ok) {
                if (state == game_state::editor) {
                    get_editor_screen().set_status(std::format("Could not fetch {}: {}", name, og::net::describe(status)));
                }

                return;
            }

            std::istringstream is(data);
            auto parsed = og::shared::load_level(name, is);
            if (!parsed) {
                return;
            }

            const auto converted = parsed->version != "GLEVNW01";

            get_editor_screen().open_level(name, std::move(*parsed), converted);

            if (state != game_state::editor) {
                state = game_state::editor;
                get_editor_screen().show();
            }
        });

        network.on_edit_save([](const std::string &level, const og::net::edit_status status, const std::string &detail) {
            get_editor_screen().handle_save_result(level, status, detail);
        });

        get_editor_screen().on_exit([] {
            get_network().send_edit_release("");
            get_editor_screen().hide();
            state = game_state::playing;
        });

        get_editor_screen().on_save_level([](const std::string &name, const std::string &content) {
            get_network().send_edit_save_level(name, content);
        });

        network.on_edit_listing([](const int category, const std::vector<std::string> &names) {
            get_editor_screen().handle_directory_listing(category, names);
        });

        get_editor_screen().on_list_directory([](const int category) {
            get_network().send_edit_list_directory(category);
        });

        get_editor_screen().on_open_level_request([](const std::string &name) {
            get_network().send_edit_request(name);
        });

        get_editor_screen().on_release_level([](const std::string &name) {
            get_network().send_edit_release(name);
        });

        network.on_weapon([](const std::string &name, const std::string &image, const std::string &source) {
            get_script_host().add_weapon(name, image, source);
        });

        get_script_host().on_client_flag([&network](const std::string &name, const std::string &text) {
            network.send_client_flag(name, text);
        });

        network.on_weapon_removed([](const std::string &name) {
            get_script_host().remove_weapon(name);
        });

        network.on_flag([](const og::net::flag_store store, const std::string &name, const std::string &value) {
            get_script_host().set_flag(store, name, value);
        });

        network.on_action([](const std::string &name, const std::string &action, const std::vector<std::string> &args) {
            get_script_host().action(name, action, args);
        });

        get_script_host().on_trigger_server([](const std::string &name, const std::string &action, const std::vector<std::string> &args) {
            get_network().send_trigger(name, action, args);
        });

        get_script_host().on_trigger_action([](const float x, const float y, const std::string &action, const std::vector<std::string> &args) {
            get_network().send_action(x, y, action, args);
        });

        network.on_login_result([](const og::net::login_status status) {
            auto &screen = get_login_screen();

            if (status != og::net::login_status::ok) {
                screen.set_status(std::string(og::net::describe(status)));
                screen.set_busy(false);

                return;
            }

            get_client_prefs().remember(pending_account, pending_password, pending_nickname, pending_save_password);

            screen.hide();
            state = game_state::playing;
        });

        network.on_create_result([](const og::net::create_status status, const std::string &message) {
            auto &screen = get_login_screen();

            if (status != og::net::create_status::ok) {
                screen.set_status(message);

                return;
            }

            screen.close_create_account();
            screen.set_fields(pending_account, pending_password, pending_nickname);
            screen.set_status("Account created. Signing in...");

            get_network().send_login();
        });

        network.on_link_lost([] {
            state = game_state::login;

            get_editor_screen().hide();
            get_script_host().clear();
            clear_gmap_window();

            auto &screen = get_login_screen();
            screen.show();
            screen.set_status("Connection lost.");
        });

        get_login_screen().on_start([network_config](const std::string &account, const std::string &password, const std::string &nickname, const bool save) {
            begin_login(network_config, account, password, nickname, save);
        });

        get_login_screen().on_create([network_config](const std::string &account, const std::string &password) {
            begin_create(network_config, account, password);
        });
    }

    {
        get_client_prefs().load();

        const auto remembered = get_client_prefs().last();

        auto &screen = get_login_screen();
        screen.set_accounts(get_client_prefs().accounts());
        screen.set_fields(remembered.account, remembered.password, remembered.nickname);
        screen.set_save_password(!remembered.password.empty());
        screen.show();
    }

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(BLACK);

        if (state == game_state::login) {
            DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), login_sky_top, login_sky_bottom);
        } else if (state == game_state::playing) {
            draw();

            if (const auto effect = get_script_host().get_screen_effect(); effect.a > 0) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), effect);
            }

            get_script_host().draw();

            current_sign->draw(SIGN_WIDTH, SIGN_HEIGHT);
        }

        if (gui) {
            gui->draw();
        }

        if (state != game_state::editor) {
            draw_diagnostics();
        }

        const auto dt = GetFrameTime();
        const auto input_done = dev_input::update(dt);
        const auto shot_done = update_screenshot(screenshot, dt);
        const auto done = input_done || shot_done;

        EndDrawing();

        if (done) {
            break;
        }

        update();
    }

    get_network().disconnect();
}

auto get_gui() -> const std::shared_ptr<gui_control> & {
    return gui;
}

auto get_local_player() -> player * {
    return local_player;
}

auto world_to_screen(const Vector2 &world) -> Vector2 {
    constexpr auto camera_offset = Vector2(16, 16);
    const auto screen_half_size = Vector2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())) / 2.0f;

    return world + screen_half_size - local_player->get_position() - camera_offset;
}

auto screen_to_world(const Vector2 &screen) -> Vector2 {
    constexpr auto camera_offset = Vector2(16, 16);
    const auto screen_half_size = Vector2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())) / 2.0f;

    return screen - screen_half_size + local_player->get_position() + camera_offset;
}

void request_level(const std::string &name) {
    if (name.empty() || has_level(name) || has_file(name)) {
        return;
    }

    get_network().request_file(name);
}

void set_gmap_window(const int cell_x, const int cell_y, const std::array<std::string, 9> &neighbours, const network::gmap_terrain &terrain) {
    current_gmap.active = true;
    current_gmap.cell_x = cell_x;
    current_gmap.cell_y = cell_y;
    current_gmap.names = neighbours;
    current_terrain = terrain;

    for (size_t i = 0; i < neighbours.size(); ++i) {
        if (neighbours[i].empty()) {
            current_gmap.levels[i] = nullptr;

            continue;
        }

        request_level(neighbours[i]);

        current_gmap.levels[i] = has_level(neighbours[i]) || has_file(neighbours[i])
                                     ? load_level(neighbours[i])
                                     : nullptr;

        fill_generated_tiles(current_gmap.levels[i],
            cell_x + (static_cast<int>(i) % 3) - 1,
            cell_y + (static_cast<int>(i) / 3) - 1);
    }

    fill_generated_tiles(current_level_info->level, cell_x, cell_y);
}

void on_level_downloaded(const std::string &name, const std::string &data) {
    auto downloaded = level::load(name, data);
    if (!downloaded) {
        return;
    }

    install_level(name, downloaded);

    for (size_t i = 0; i < current_gmap.names.size(); ++i) {
        if (current_gmap.names[i] == name) {
            fill_generated_tiles(downloaded,
                current_gmap.cell_x + (static_cast<int>(i) % 3) - 1,
                current_gmap.cell_y + (static_cast<int>(i) / 3) - 1);
        }
    }

    if (pending_level == name) {
        pending_level.clear();
        current_level_info->level = downloaded;

        refresh_tileset();

        local_player->try_move_from_wall(local_player->get_position());
        get_script_host().player_entered_level();
    }

    for (size_t i = 0; i < current_gmap.names.size(); ++i) {
        if (current_gmap.names[i] == name) {
            current_gmap.levels[i] = downloaded;
        }
    }
}

void on_level_reloaded(const std::string &name, const std::string &data) {
    auto reloaded = level::load(name, data);
    if (!reloaded) {
        return;
    }

    install_level(name, reloaded);

    for (size_t i = 0; i < current_gmap.names.size(); ++i) {
        if (og::shared::iequals(current_gmap.names[i], name)) {
            fill_generated_tiles(reloaded,
                current_gmap.cell_x + (static_cast<int>(i) % 3) - 1,
                current_gmap.cell_y + (static_cast<int>(i) / 3) - 1);
            current_gmap.levels[i] = reloaded;
        }
    }

    if (current_level_info->level != nullptr && og::shared::iequals(current_level_info->level->get_name(), name)) {
        current_level_info->level = reloaded;

        refresh_tileset();
    }
}

void clear_gmap_window() {
    current_gmap = gmap_window{};
}

auto get_current_level() -> const std::shared_ptr<level> & {
    return current_level_info->level;
}

void refresh_tileset() {
    if (current_level_info->level != nullptr) {
        current_level_info->tileset = tileset_for_level(current_level_info->level->get_name());
    }
}

void change_level(const std::string &level_name) {
    if (!has_level(level_name) && !has_file(level_name)) {
        pending_level = level_name;

        get_network().request_file(level_name);

        return;
    }

    const auto level = load_level(level_name);

    if (level == nullptr) {
        return;
    }

    pending_level.clear();
    current_level_info->level = level;

    refresh_tileset();
}

namespace {
    auto resolve_across_gmap(const Vector2 pt) -> std::pair<const level *, Vector2> {
        if (pt.x >= 0.0f && pt.y >= 0.0f && pt.x < level_pixel_size && pt.y < level_pixel_size) {
            return {current_level_info->level.get(), pt};
        }

        if (!current_gmap.active) {
            return {nullptr, pt};
        }

        const auto offset_x = static_cast<int>(std::floor(pt.x / level_pixel_size));
        const auto offset_y = static_cast<int>(std::floor(pt.y / level_pixel_size));

        if (offset_x < -1 || offset_x > 1 || offset_y < -1 || offset_y > 1) {
            return {nullptr, pt};
        }

        const auto &neighbour = current_gmap.levels[((offset_y + 1) * 3) + offset_x + 1];

        return {
            neighbour.get(),
            {pt.x - (static_cast<float>(offset_x) * level_pixel_size),
                    pt.y - (static_cast<float>(offset_y) * level_pixel_size)},
        };
    }
}

auto on_wall(const Rectangle rect) -> bool {
    for (auto y = rect.y; y < rect.y + rect.height + tile_size; y += tile_size) {
        for (auto x = rect.x; x < rect.x + rect.width + tile_size; x += tile_size) {
            if (on_wall(Vector2{std::min(x, rect.x + rect.width), std::min(y, rect.y + rect.height)})) {
                return true;
            }
        }
    }

    return false;
}

auto on_wall(const Vector2 pt) -> bool {
    const auto [target, local] = resolve_across_gmap(pt);
    if (target == nullptr) {
        return true;
    }

    return target->on_wall(current_level_info->tileset, local);
}

auto get_tile_type(const int x, const int y) -> int {
    const auto [target, local] = resolve_across_gmap({static_cast<float>(x), static_cast<float>(y)});
    if (target == nullptr) {
        return tile_type::passable;
    }

    return target->get_tile_type(current_level_info->tileset,
        static_cast<int>(local.x),
        static_cast<int>(local.y));
}

void show_sign(const std::string &str) {
    current_sign->show(str);
}

void spawn_thrown_item(const carry_object_type type, const Vector2 origin, const direction dir) {
    if (type == carry_object_type::none) {
        return;
    }

    active_thrown_items.emplace_back(type, origin, dir);
}

void spawn_leaps(leap_effect_type type, Vector2 origin) {
    play_sound("crush.wav");

    active_leaps.emplace_back(type, origin);
}
