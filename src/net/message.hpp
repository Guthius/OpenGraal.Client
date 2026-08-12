#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace og::net {
    inline constexpr uint16_t protocol_version = 3;
    inline constexpr uint32_t max_message_size = 8u * 1024u * 1024u;

    enum class message_id : uint16_t {
        // client -> server
        login = 1,
        move = 2,
        chat = 3,
        set_property = 4,
        request_file = 5,
        ping = 6,
        trigger_server = 7,
        trigger_action = 8,
        flag_changed = 9,
        create_account = 10,
        edit_request = 11,
        edit_release = 12,
        edit_list_directory = 13,
        edit_fetch_level = 14,
        edit_save_level = 15,

        // server -> client
        login_result = 100,
        disconnect = 101,
        level_enter = 102,
        level_board = 103,
        player_add = 104,
        player_update = 105,
        player_remove = 106,
        player_chat = 107,
        player_identity = 108,
        file_data = 109,
        file_missing = 110,
        pong = 111,
        weapon_script = 112,
        trigger_client = 113,
        npc_add = 114,
        npc_update = 115,
        npc_remove = 116,
        flag_set = 117,
        level_gmap = 118,
        item_add = 119,
        item_remove = 120,
        server_warp = 121,
        weapon_remove = 122,
        create_account_result = 123,
        edit_response = 124,
        edit_directory_listing = 125,
        edit_level_content = 126,
        edit_save_result = 127,
        level_reload = 128,
    };

    enum class edit_status : uint8_t {
        ok = 0,
        not_developer = 1,
        locked = 2,
        not_found = 3,
        invalid = 4,
        io_error = 5,
        not_locked = 6,
        bad_name = 7,
    };

    enum class flag_store : uint8_t {
        serverr = 0,
        clientr = 1,
        client = 2,
    };

    struct item_state {
        uint32_t id = 0;
        float x = 0.0f;
        float y = 0.0f;
        std::string name;
        std::string image;
    };

    struct npc_state {
        uint32_t id = 0;
        float x = 0.0f;
        float y = 0.0f;
        std::string image;
        std::string animation;
        std::string chat;
        std::string nickname;
    };

    enum class login_status : uint8_t {
        ok = 0,
        bad_version = 1,
        bad_credentials = 2,
        already_online = 3,
        server_full = 4,
    };

    enum class create_status : uint8_t {
        ok = 0,
        bad_version = 1,
        bad_email = 2,
        weak_password = 3,
        already_exists = 4,
        not_allowed = 5,
    };

    [[nodiscard]] auto describe(login_status status) -> std::string_view;
    [[nodiscard]] auto describe(create_status status) -> std::string_view;
    [[nodiscard]] auto describe(edit_status status) -> std::string_view;

    inline constexpr size_t player_attr_count = 31;

    struct player_state {
        uint16_t id = 0;
        float x = 0.0f;
        float y = 0.0f;
        uint8_t direction = 0;
        std::string nickname;
        std::string animation = "idle";
        std::string head = "head0.png";
        std::string body = "body.png";
        std::vector<std::string> attr = std::vector<std::string>(player_attr_count);
        bool visible = true;
    };

    class packet_writer {
      public:
        explicit packet_writer(message_id id);

        auto put_u8(uint8_t value) -> packet_writer &;
        auto put_u16(uint16_t value) -> packet_writer &;
        auto put_u32(uint32_t value) -> packet_writer &;
        auto put_float(float value) -> packet_writer &;
        auto put_string(std::string_view value) -> packet_writer &;
        auto put_bytes(const std::vector<uint8_t> &value) -> packet_writer &;

        [[nodiscard]] auto frame() const -> std::vector<uint8_t>;

      private:
        std::vector<uint8_t> body_;
    };

    class packet_reader {
      public:
        packet_reader(message_id id, std::vector<uint8_t> body);

        [[nodiscard]] auto id() const -> message_id { return id_; }
        [[nodiscard]] auto failed() const -> bool { return failed_; }
        [[nodiscard]] auto remaining() const -> size_t { return body_.size() - position_; }

        auto get_u8() -> uint8_t;
        auto get_u16() -> uint16_t;
        auto get_u32() -> uint32_t;
        auto get_float() -> float;
        auto get_string() -> std::string;
        auto get_bytes() -> std::vector<uint8_t>;

      private:
        auto take(size_t count) -> const uint8_t *;

        message_id id_;
        std::vector<uint8_t> body_;
        size_t position_ = 0;
        bool failed_ = false;
    };

    void write_player_state(packet_writer &writer, const player_state &state);
    auto read_player_state(packet_reader &reader) -> player_state;

    void write_npc_state(packet_writer &writer, const npc_state &state);
    auto read_npc_state(packet_reader &reader) -> npc_state;

    void write_item_state(packet_writer &writer, const item_state &state);
    auto read_item_state(packet_reader &reader) -> item_state;
}
