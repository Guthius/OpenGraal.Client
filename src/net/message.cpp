#include <net/message.hpp>

#include <cstring>

using namespace std;

namespace og::net {
    namespace {

        template <typename T>
        void append(vector<uint8_t> &buffer, const T value) {
            for (size_t i = 0; i < sizeof(T); ++i) {
                buffer.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
            }
        }

        template <typename T>
        auto read(const uint8_t *data) -> T {
            T value = 0;
            for (size_t i = 0; i < sizeof(T); ++i) {
                value |= static_cast<T>(data[i]) << (i * 8);
            }

            return value;
        }
    }

    packet_writer::packet_writer(const message_id id) {
        body_.reserve(64);
        append<uint16_t>(body_, static_cast<uint16_t>(id));
    }

    auto packet_writer::put_u8(const uint8_t value) -> packet_writer & {
        body_.push_back(value);

        return *this;
    }

    auto packet_writer::put_u16(const uint16_t value) -> packet_writer & {
        append(body_, value);

        return *this;
    }

    auto packet_writer::put_u32(const uint32_t value) -> packet_writer & {
        append(body_, value);

        return *this;
    }

    auto packet_writer::put_float(const float value) -> packet_writer & {
        uint32_t bits = 0;
        memcpy(&bits, &value, sizeof(bits));

        return put_u32(bits);
    }

    auto packet_writer::put_string(const string_view value) -> packet_writer & {
        put_u16(static_cast<uint16_t>(min<size_t>(value.size(), UINT16_MAX)));
        body_.insert(body_.end(), value.begin(), value.begin() + static_cast<ptrdiff_t>(min<size_t>(value.size(), UINT16_MAX)));

        return *this;
    }

    auto packet_writer::put_bytes(const vector<uint8_t> &value) -> packet_writer & {
        put_u32(static_cast<uint32_t>(value.size()));
        body_.insert(body_.end(), value.begin(), value.end());

        return *this;
    }

    auto packet_writer::frame() const -> vector<uint8_t> {
        vector<uint8_t> framed;
        framed.reserve(body_.size() + sizeof(uint32_t));

        append<uint32_t>(framed, static_cast<uint32_t>(body_.size()));
        framed.insert(framed.end(), body_.begin(), body_.end());

        return framed;
    }

    packet_reader::packet_reader(const message_id id, vector<uint8_t> body)
        : id_(id), body_(std::move(body)) {
    }

    auto packet_reader::take(const size_t count) -> const uint8_t * {
        if (failed_ || remaining() < count) {
            failed_ = true;

            return nullptr;
        }

        const auto *data = body_.data() + position_;
        position_ += count;

        return data;
    }

    auto packet_reader::get_u8() -> uint8_t {
        const auto *data = take(1);

        return data == nullptr ? 0 : *data;
    }

    auto packet_reader::get_u16() -> uint16_t {
        const auto *data = take(sizeof(uint16_t));

        return data == nullptr ? 0 : read<uint16_t>(data);
    }

    auto packet_reader::get_u32() -> uint32_t {
        const auto *data = take(sizeof(uint32_t));

        return data == nullptr ? 0 : read<uint32_t>(data);
    }

    auto packet_reader::get_float() -> float {
        const auto bits = get_u32();

        float value = 0.0f;
        memcpy(&value, &bits, sizeof(value));

        return value;
    }

    auto packet_reader::get_string() -> string {
        const auto length = get_u16();
        const auto *data = take(length);

        return data == nullptr ? string{} : string(reinterpret_cast<const char *>(data), length);
    }

    auto packet_reader::get_bytes() -> vector<uint8_t> {
        const auto length = get_u32();
        if (length > max_message_size) {
            failed_ = true;

            return {};
        }

        const auto *data = take(length);

        return data == nullptr ? vector<uint8_t>{} : vector(data, data + length);
    }

    void write_player_state(packet_writer &writer, const player_state &state) {
        writer.put_u16(state.id)
            .put_float(state.x)
            .put_float(state.y)
            .put_u8(state.direction)
            .put_string(state.nickname)
            .put_string(state.animation)
            .put_string(state.head)
            .put_string(state.body)
            .put_u8(state.visible ? 1 : 0);

        uint8_t set = 0;
        for (const auto &value : state.attr) {
            set += value.empty() ? 0 : 1;
        }

        writer.put_u8(set);

        for (size_t i = 0; i < state.attr.size(); ++i) {
            if (!state.attr[i].empty()) {
                writer.put_u8(static_cast<uint8_t>(i)).put_string(state.attr[i]);
            }
        }
    }

    auto read_player_state(packet_reader &reader) -> player_state {
        player_state state;

        state.id = reader.get_u16();
        state.x = reader.get_float();
        state.y = reader.get_float();
        state.direction = reader.get_u8();
        state.nickname = reader.get_string();
        state.animation = reader.get_string();
        state.head = reader.get_string();
        state.body = reader.get_string();
        state.visible = reader.get_u8() != 0;

        const auto set = reader.get_u8();
        for (uint8_t i = 0; i < set && !reader.failed(); ++i) {
            const auto index = reader.get_u8();
            auto value = reader.get_string();

            if (index < state.attr.size()) {
                state.attr[index] = std::move(value);
            }
        }

        return state;
    }
}

namespace og::net {
    void write_npc_state(packet_writer &writer, const npc_state &state) {
        writer.put_u32(state.id)
            .put_float(state.x)
            .put_float(state.y)
            .put_string(state.image)
            .put_string(state.animation)
            .put_string(state.chat)
            .put_string(state.nickname);
    }

    auto read_npc_state(packet_reader &reader) -> npc_state {
        npc_state state;

        state.id = reader.get_u32();
        state.x = reader.get_float();
        state.y = reader.get_float();
        state.image = reader.get_string();
        state.animation = reader.get_string();
        state.chat = reader.get_string();
        state.nickname = reader.get_string();

        return state;
    }
}

namespace og::net {
    void write_item_state(packet_writer &writer, const item_state &state) {
        writer.put_u32(state.id)
            .put_float(state.x)
            .put_float(state.y)
            .put_string(state.name)
            .put_string(state.image);
    }

    auto read_item_state(packet_reader &reader) -> item_state {
        item_state state;

        state.id = reader.get_u32();
        state.x = reader.get_float();
        state.y = reader.get_float();
        state.name = reader.get_string();
        state.image = reader.get_string();

        return state;
    }

    auto describe(const login_status status) -> std::string_view {
        switch (status) {
        case login_status::ok:              return "connected";
        case login_status::bad_version:     return "version mismatch";
        case login_status::bad_credentials: return "bad account or password";
        case login_status::already_online:  return "already online";
        case login_status::server_full:     return "server full";
        }

        return "refused";
    }

    auto describe(const create_status status) -> std::string_view {
        switch (status) {
        case create_status::ok:             return "account created";
        case create_status::bad_version:    return "version mismatch";
        case create_status::bad_email:      return "that is not a valid e-mail address";
        case create_status::weak_password:  return "password is too short";
        case create_status::already_exists: return "an account already exists for that address";
        case create_status::not_allowed:    return "this server is not accepting new accounts";
        }

        return "could not create the account";
    }

    auto describe(const edit_status status) -> std::string_view {
        switch (status) {
        case edit_status::ok:            return "ok";
        case edit_status::not_developer: return "this account has no developer rights";
        case edit_status::locked:        return "the level is being edited by";
        case edit_status::not_found:     return "the level was not found";
        case edit_status::invalid:       return "the level failed validation:";
        case edit_status::io_error:      return "the server could not write the level";
        case edit_status::not_locked:    return "you do not hold the edit lock";
        case edit_status::bad_name:      return "that name is not allowed";
        }

        return "refused";
    }
}
