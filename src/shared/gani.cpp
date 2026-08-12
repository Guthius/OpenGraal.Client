#include <shared/gani.hpp>

#include <shared/text.hpp>

#include <format>
#include <fstream>

using namespace std;

namespace og::shared {
    namespace {
        constexpr string_view gani_signature = "GANI0001";
        constexpr int gani_sprite_token_count = 7;
        constexpr int gani_sprite_ref_token_count = 3;
        constexpr int gani_wait_scale = 10;

        auto parse_source(const string_view name, int &index) -> gani_sprite_source {
            index = 0;

            if (name == "SPRITES") return gani_sprite_source::sprites;
            if (name == "HEAD") return gani_sprite_source::head;
            if (name == "BODY") return gani_sprite_source::body;
            if (name == "SWORD") return gani_sprite_source::sword;
            if (name == "SHIELD") return gani_sprite_source::shield;
            if (name == "HORSE") return gani_sprite_source::horse;

            if (name.starts_with("ATTR")) {
                index = to_int(name.substr(4));

                return gani_sprite_source::attribute;
            }

            if (name.starts_with("PARAM")) {
                index = to_int(name.substr(5));

                return gani_sprite_source::parameter;
            }

            return gani_sprite_source::file;
        }

        void parse_sprite(gani_data &gani, const vector<string> &tokens) {
            int source_index = 0;
            const auto source = parse_source(tokens[2], source_index);

            auto sprite = gani_sprite{
                .id = to_int(tokens[1]),
                .source = source,
                .source_index = source_index,
                .image = source == gani_sprite_source::file ? tokens[2] : string{},
                .x = to_int(tokens[3]),
                .y = to_int(tokens[4]),
                .width = to_int(tokens[5]),
                .height = to_int(tokens[6]),
            };

            for (size_t i = gani_sprite_token_count; i < tokens.size(); ++i) {
                sprite.description += (i > gani_sprite_token_count ? " " : "") + tokens[i];
            }

            gani.sprites[sprite.id] = std::move(sprite);
        }

        auto parse_sprite_refs(const string_view line) -> vector<gani_sprite_ref> {
            vector<gani_sprite_ref> refs;

            for (const auto &entry : split(line, ',')) {
                const auto tokens = split_whitespace(entry);
                if (tokens.size() != gani_sprite_ref_token_count) {
                    continue;
                }

                refs.push_back(gani_sprite_ref{
                    .sprite_id = to_int(tokens[0]),
                    .x = to_float(tokens[1]),
                    .y = to_float(tokens[2]),
                });
            }

            return refs;
        }

        void parse_frames(gani_data &gani, istream &is) {
            string line;

            while (getline(is, line)) {
                auto trimmed = trim(line);
                if (trimmed == "ANIEND") {
                    return;
                }

                if (trimmed.empty()) {
                    continue;
                }

                auto frame = gani_frame{};
                frame.directions[0] = parse_sprite_refs(trimmed);

                if (!gani.single_direction) {
                    for (int i = 1; i < gani_direction_count; ++i) {
                        if (!getline(is, line)) {
                            break;
                        }

                        frame.directions[i] = parse_sprite_refs(trim(line));
                    }
                }

                while (getline(is, line)) {
                    trimmed = trim(line);

                    if (trimmed.empty() || trimmed == "ANIEND") {
                        break;
                    }

                    const auto tokens = split_whitespace(trimmed);
                    if (tokens.empty()) {
                        break;
                    }

                    if (tokens[0] == "WAIT" && tokens.size() == 2) {
                        frame.duration = to_float(tokens[1]) / gani_wait_scale;
                    } else if (tokens[0] == "PLAYSOUND" && tokens.size() == 4) {
                        frame.sound = tokens[1];
                        frame.sound_x = to_float(tokens[2]);
                        frame.sound_y = to_float(tokens[3]);
                    }
                }

                gani.frames.push_back(std::move(frame));

                if (trim(line) == "ANIEND") {
                    return;
                }
            }
        }

        auto read_remaining(istream &is) -> string {
            string script;
            string line;

            while (getline(is, line)) {
                if (trim(line) == "SCRIPTEND") {
                    break;
                }

                script += line;
                script += '\n';
            }

            return script;
        }
    }

    auto gani_data::find_sprite(const int id) const -> const gani_sprite * {
        const auto match = sprites.find(id);

        return match == sprites.end() ? nullptr : &match->second;
    }

    auto load_gani(string name, istream &is) -> result<gani_data> {
        auto gani = gani_data{.name = std::move(name)};

        string line;
        while (getline(is, line)) {
            const auto trimmed = trim(line);
            if (trimmed.empty()) {
                continue;
            }

            const auto tokens = split_whitespace(trimmed);
            const auto &keyword = tokens[0];

            if (keyword == gani_signature) {
                continue;
            }

            if (keyword == "SPRITE" && tokens.size() >= gani_sprite_token_count) {
                parse_sprite(gani, tokens);
            } else if (keyword == "ATTACHSPRITE" && tokens.size() == 5) {
                gani.attachments.push_back(gani_attachment{
                    .sprite_id = to_int(tokens[1]),
                    .attached_sprite_id = to_int(tokens[2]),
                    .x = to_float(tokens[3]),
                    .y = to_float(tokens[4]),
                });
            } else if (keyword == "SINGLEDIRECTION") {
                gani.single_direction = true;
            } else if (keyword == "CONTINUOUS") {
                gani.continuous = true;
            } else if (keyword == "LOOP") {
                gani.loop = true;
            } else if (keyword == "SETBACKTO" && tokens.size() >= 2) {
                gani.set_back_to = tokens[1];
            } else if (keyword == "DEFAULTHEAD" && tokens.size() >= 2) {
                gani.default_head = tokens[1];
            } else if (keyword == "DEFAULTBODY" && tokens.size() >= 2) {
                gani.default_body = tokens[1];
            } else if (keyword.starts_with("DEFAULTATTR") && tokens.size() >= 2) {
                const auto index = to_int(string_view(keyword).substr(11));
                if (index >= 1 && index <= gani_attribute_count) {
                    gani.default_attributes[index] = tokens[1];
                }
            } else if (keyword == "ANI") {
                parse_frames(gani, is);
            } else if (keyword == "SCRIPT") {
                gani.script = read_remaining(is);
            }
        }

        return gani;
    }

    auto load_gani(const filesystem::path &path) -> result<gani_data> {
        if (!is_regular_file(path)) {
            return make_error(format("'{}' is not a file", path.string()));
        }

        ifstream ifs(path, ios::binary);
        if (!ifs) {
            return make_error(format("could not open '{}'", path.string()));
        }

        return load_gani(path.filename().string(), ifs);
    }
}
