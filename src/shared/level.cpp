#include <shared/level.hpp>

#include <shared/sign_text.hpp>
#include <shared/text.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <span>
#include <sstream>

using namespace std;

namespace og::shared {
    namespace {
        constexpr string_view board_alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        constexpr int graal_board_bits_v0 = 12;
        constexpr int graal_board_bits_v1 = 13;

        auto decode_tile(const char left, const char right) -> tile_id {
            const auto high = board_alphabet.find(left);
            const auto low = board_alphabet.find(right);

            if (high == string_view::npos || low == string_view::npos) {
                return 0;
            }

            return static_cast<tile_id>((high << 6) | low);
        }

        auto layer_for(level_data &level, const int index) -> level_layer & {
            for (auto &layer : level.layers) {
                if (layer.index == index) {
                    return layer;
                }
            }

            level.layers.push_back(level_layer{
                .index = index,
                .tiles = vector<tile_id>(level_tile_count, 0),
            });

            return level.layers.back();
        }

        auto parse_link(const vector<string> &tokens) -> level_link {
            const auto offset = tokens.size() - 7;

            auto destination = tokens[0];
            for (size_t i = 0; i < offset; ++i) {
                destination += " " + tokens[1 + i];
            }

            return level_link{
                .destination = destination,
                .x = to_int(tokens[1 + offset]),
                .y = to_int(tokens[2 + offset]),
                .width = to_int(tokens[3 + offset]),
                .height = to_int(tokens[4 + offset]),
                .destination_x = tokens[5 + offset],
                .destination_y = tokens[6 + offset],
            };
        }

        void parse_board(level_data &level, const vector<string> &tokens) {
            const auto x = to_int(tokens[1]);
            const auto y = to_int(tokens[2]);
            const auto width = to_int(tokens[3]);
            const auto index = to_int(tokens[4]);
            const auto &data = tokens[5];

            if (x < 0 || y < 0 || y >= level_height || width <= 0 || x + width > level_width) {
                return;
            }

            if (static_cast<int>(data.size()) < width * 2) {
                return;
            }

            auto &layer = layer_for(level, index);
            for (int i = 0; i < width; ++i) {
                layer.tiles[(y * level_width) + x + i] = decode_tile(data[i * 2], data[(i * 2) + 1]);
            }
        }

        auto collect_until(istream &is, const string_view terminator, string &line) -> vector<string> {
            vector<string> lines;

            while (getline(is, line)) {
                if (trim(line) == terminator) {
                    break;
                }

                lines.push_back(line);
            }

            return lines;
        }

        auto join_lines(const vector<string> &lines) -> string {
            string joined;

            for (const auto &line : lines) {
                joined += line;
                joined += '\n';
            }

            return joined;
        }

        auto load_nw(level_data &level, istream &is) -> result<level_data> {
            string line;

            while (getline(is, line)) {
                auto tokens = split_whitespace(line);
                if (tokens.empty()) {
                    continue;
                }

                const auto &keyword = tokens[0];

                if (keyword == "BOARD" && tokens.size() == 6) {
                    parse_board(level, tokens);
                } else if (keyword == "LINK" && tokens.size() >= 8) {
                    level.links.push_back(parse_link(vector(tokens.begin() + 1, tokens.end())));
                } else if (keyword == "CHEST" && tokens.size() == 5) {
                    level.chests.push_back(level_chest{
                        .x = to_int(tokens[1]),
                        .y = to_int(tokens[2]),
                        .item = tokens[3],
                        .sign_index = to_int(tokens[4]),
                    });
                } else if (keyword == "SIGN" && tokens.size() == 3) {
                    level.signs.push_back(level_sign{
                        .x = to_int(tokens[1]),
                        .y = to_int(tokens[2]),
                        .text = join_lines(collect_until(is, "SIGNEND", line)),
                    });
                } else if (keyword == "NPC" && tokens.size() >= 4) {
                    const auto offset = tokens.size() - 4;

                    auto image = tokens[1];
                    for (size_t i = 0; i < offset; ++i) {
                        image += " " + tokens[2 + i];
                    }

                    level.npcs.push_back(level_npc{
                        .image = image == "-" ? "" : image,
                        .x = to_float(tokens[2 + offset]),
                        .y = to_float(tokens[3 + offset]),
                        .script = join_lines(collect_until(is, "NPCEND", line)),
                    });
                } else if (keyword == "BADDY" && tokens.size() == 4) {
                    level.baddies.push_back(level_baddy{
                        .x = to_int(tokens[1]),
                        .y = to_int(tokens[2]),
                        .type = to_int(tokens[3]),
                        .verses = collect_until(is, "BADDYEND", line),
                    });
                } else if (keyword == "HEIGHTS") {
                    level.raw_lines.push_back(line);
                    for (const auto &kept : collect_until(is, "HEIGHTSEND", line)) {
                        level.raw_lines.push_back(kept);
                    }
                    level.raw_lines.emplace_back("HEIGHTSEND");
                } else {
                    level.raw_lines.push_back(line);
                }
            }

            return level;
        }

        class byte_reader {
          public:
            explicit byte_reader(string data) : data_(std::move(data)) {
            }

            [[nodiscard]] auto done() const -> bool { return position_ >= data_.size(); }
            [[nodiscard]] auto remaining() const -> size_t { return data_.size() - position_; }

            auto read_byte() -> uint8_t {
                return done() ? 0 : static_cast<uint8_t>(data_[position_++]);
            }

            auto read_signed() -> int {
                return static_cast<int8_t>(read_byte());
            }

            auto read_line() -> string {
                const auto end = data_.find('\n', position_);
                if (end == string::npos) {
                    auto line = data_.substr(position_);
                    position_ = data_.size();

                    return line;
                }

                auto line = data_.substr(position_, end - position_);
                position_ = end + 1;

                return line;
            }

          private:
            string data_;
            size_t position_ = 0;
        };

        auto graal_char(const string &str, const size_t index) -> int {
            return index < str.size() ? static_cast<uint8_t>(str[index]) - 32 : 0;
        }

        void load_graal_board(level_data &level, byte_reader &reader, const int version) {
            const auto bits = version > 0 ? graal_board_bits_v1 : graal_board_bits_v0;
            const auto code_mask = version > 0 ? 0x1FFF : 0xFFF;
            const auto control_bit = version > 0 ? 0x1000 : 0x800;

            auto &layer = layer_for(level, 0);

            int read = 0;
            unsigned int buffer = 0;
            int index = 0;
            int count = 1;
            bool double_mode = false;
            tile_id pending = -1;

            while (index < level_tile_count && !reader.done()) {
                while (read < bits) {
                    buffer |= static_cast<unsigned int>(reader.read_byte()) << read;
                    read += 8;
                }

                const auto code = static_cast<int>(buffer & static_cast<unsigned int>(code_mask));
                buffer >>= bits;
                read -= bits;

                if ((code & control_bit) != 0) {
                    double_mode = (code & 0x100) != 0;
                    count = code & 0xFF;
                    continue;
                }

                if (count == 1) {
                    layer.tiles[index++] = static_cast<tile_id>(code);
                    continue;
                }

                if (double_mode) {
                    if (pending == -1) {
                        pending = static_cast<tile_id>(code);
                        continue;
                    }

                    for (int i = 0; i < count && index < level_tile_count - 1; ++i) {
                        layer.tiles[index++] = pending;
                        layer.tiles[index++] = static_cast<tile_id>(code);
                    }

                    pending = -1;
                    double_mode = false;
                } else {
                    for (int i = 0; i < count && index < level_tile_count; ++i) {
                        layer.tiles[index++] = static_cast<tile_id>(code);
                    }
                }

                count = 1;
            }
        }

        auto load_graal(level_data &level, byte_reader &reader, const int version) -> result<level_data> {
            load_graal_board(level, reader, version);

            while (!reader.done()) {
                const auto line = trim(reader.read_line());
                if (line.empty() || line == "#") {
                    break;
                }

                auto tokens = split_whitespace(line);
                if (tokens.size() >= 7) {
                    level.links.push_back(parse_link(tokens));
                }
            }

            while (!reader.done()) {
                const auto x = reader.read_signed();
                const auto y = reader.read_signed();
                const auto type = reader.read_signed();

                if (x == -1 && y == -1 && type == -1) {
                    reader.read_line();
                    break;
                }

                level.baddies.push_back(level_baddy{
                    .x = x,
                    .y = y,
                    .type = type,
                    .verses = split(reader.read_line(), '\\'),
                });
            }

            while (!reader.done()) {
                const auto line = reader.read_line();
                if (trim(line).empty() || trim(line) == "#") {
                    break;
                }

                const auto separator = line.find('#', 2);
                const auto image_end = separator == string::npos ? line.size() : separator;

                level.npcs.push_back(level_npc{
                    .image = trim(line.substr(2, image_end - 2)),
                    .x = static_cast<float>(graal_char(line, 0)),
                    .y = static_cast<float>(graal_char(line, 1)),
                    .script = separator == string::npos ? "" : line.substr(separator + 1),
                });
            }

            if (version > 0) {
                while (!reader.done()) {
                    const auto line = reader.read_line();
                    if (trim(line).empty() || trim(line) == "#") {
                        break;
                    }

                    level.chests.push_back(level_chest{
                        .x = graal_char(line, 0),
                        .y = graal_char(line, 1),
                        .item = to_string(graal_char(line, 2)),
                        .sign_index = graal_char(line, 3),
                    });
                }
            }

            while (!reader.done()) {
                const auto line = reader.read_line();
                if (trim(line).empty()) {
                    break;
                }

                level.signs.push_back(level_sign{
                    .x = graal_char(line, 0),
                    .y = graal_char(line, 1),
                    .text = decode_sign_text(line.substr(min<size_t>(2, line.size()))),
                });
            }

            return level;
        }

        auto graal_version(const string_view version) -> int {
            if (version == "GR-V1.00") return 0;
            if (version == "GR-V1.01") return 1;
            if (version == "GR-V1.02") return 2;
            if (version == "GR-V1.03") return 3;

            return -1;
        }
    }

    auto level_data::find_layer(const int index) const -> const level_layer * {
        const auto match = ranges::find(layers, index, &level_layer::index);

        return match == layers.end() ? nullptr : &*match;
    }

    auto level_data::tiles() const -> const vector<tile_id> * {
        const auto *layer = find_layer(0);

        return layer == nullptr ? nullptr : &layer->tiles;
    }

    auto load_level(string name, istream &is) -> result<level_data> {
        string version(8, '\0');

        is.read(version.data(), 8);
        if (is.gcount() != 8) {
            return make_error(format("'{}' is too short to be a level", name));
        }

        auto level = level_data{
            .name = std::move(name),
            .version = version,
        };

        if (version == "GLEVNW01") {
            return load_nw(level, is);
        }

        if (const auto graal = graal_version(version); graal != -1) {
            ostringstream buffer;
            buffer << is.rdbuf();

            auto reader = byte_reader(buffer.str());

            return load_graal(level, reader, graal);
        }

        return make_error(format("'{}' has an unsupported level version '{}'", level.name, version));
    }

    auto load_level(const filesystem::path &path) -> result<level_data> {
        if (!is_regular_file(path)) {
            return make_error(format("'{}' is not a file", path.string()));
        }

        ifstream ifs(path, ios::binary);
        if (!ifs) {
            return make_error(format("could not open '{}'", path.string()));
        }

        return load_level(path.filename().string(), ifs);
    }

    namespace {
        auto row_text(const vector<tile_id> &tiles, const int y) -> string {
            string text(level_width * 2, 'A');

            for (int x = 0; x < level_width; ++x) {
                const auto tile = static_cast<uint16_t>(tiles[(y * level_width) + x]) & 0xFFF;
                text[x * 2] = board_alphabet[tile >> 6];
                text[(x * 2) + 1] = board_alphabet[tile & 0x3F];
            }

            return text;
        }

        auto row_is_empty(const vector<tile_id> &tiles, const int y) -> bool {
            const auto row = span(tiles).subspan(y * level_width, level_width);

            return ranges::all_of(row, [](const tile_id tile) { return tile == 0; });
        }

        void write_block(ostream &os, const string &text, const string_view terminator) {
            os << text;
            if (!text.empty() && text.back() != '\n') {
                os << '\n';
            }
            os << terminator << '\n';
        }
    }

    auto save_level(const level_data &level, ostream &os) -> void {
        os << "GLEVNW01\n";

        auto indices = vector<int>{};
        for (const auto &layer : level.layers) {
            indices.push_back(layer.index);
        }
        ranges::sort(indices);

        for (const auto index : indices) {
            const auto &tiles = level.find_layer(index)->tiles;

            for (int y = 0; y < level_height; ++y) {
                if (index == 0 || !row_is_empty(tiles, y)) {
                    os << format("BOARD 0 {} {} {} {}\n", y, level_width, index, row_text(tiles, y));
                }
            }
        }

        for (const auto &link : level.links) {
            os << format("LINK {} {} {} {} {} {} {}\n", link.destination, link.x, link.y, link.width, link.height,
                link.destination_x, link.destination_y);
        }

        for (const auto &chest : level.chests) {
            os << format("CHEST {} {} {} {}\n", chest.x, chest.y, chest.item, chest.sign_index);
        }

        for (const auto &sign : level.signs) {
            os << format("SIGN {} {}\n", sign.x, sign.y);
            write_block(os, sign.text, "SIGNEND");
        }

        for (const auto &npc : level.npcs) {
            os << format("NPC {} {} {}\n", npc.image.empty() ? "-" : npc.image, npc.x, npc.y);
            write_block(os, npc.script, "NPCEND");
        }

        for (const auto &baddy : level.baddies) {
            os << format("BADDY {} {} {}\n", baddy.x, baddy.y, baddy.type);
            for (const auto &verse : baddy.verses) {
                os << verse << '\n';
            }
            os << "BADDYEND\n";
        }

        for (const auto &line : level.raw_lines) {
            os << line << '\n';
        }
    }

    auto save_level_text(const level_data &level) -> string {
        ostringstream os;
        save_level(level, os);

        return os.str();
    }
}
