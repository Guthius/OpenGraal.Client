#include <shared/container.hpp>

#include <shared/text.hpp>

#include <format>
#include <fstream>

using namespace std;

namespace og::shared {
    namespace {
        constexpr string_view weapon_signature = "GRAWP001";
        constexpr string_view npc_signature = "GRNPC001";
        constexpr string_view name_escape = "%047";

        auto split_key(const string_view line) -> pair<string, string> {
            const auto separator = line.find(' ');
            if (separator == string_view::npos) {
                return {string(line), {}};
            }

            return {string(line.substr(0, separator)), trim(line.substr(separator + 1))};
        }

        auto read_body(istream &is, const string_view terminator) -> string {
            string body;
            string line;

            while (getline(is, line)) {
                if (trim(line) == terminator) {
                    break;
                }

                body += line;
                body += '\n';
            }

            return body;
        }

        auto has_signature(istream &is, const string_view signature) -> bool {
            string line;

            return getline(is, line) && trim(line) == signature;
        }
    }

    auto npc_container::attribute(const string_view key, const string_view fallback) const -> string {
        const auto match = attributes.find(string(key));

        return match == attributes.end() ? string(fallback) : match->second;
    }

    auto npc_container::attribute_int(const string_view key, const int fallback) const -> int {
        const auto value = attribute(key);

        return value.empty() ? fallback : to_int(value, fallback);
    }

    auto npc_container::attribute_float(const string_view key, const float fallback) const -> float {
        const auto value = attribute(key);

        return value.empty() ? fallback : to_float(value, fallback);
    }

    auto decode_container_name(const string_view filename) -> string {
        string name(filename);

        for (auto index = name.find(name_escape); index != string::npos; index = name.find(name_escape, index + 1)) {
            name.replace(index, name_escape.size(), "/");
        }

        return name;
    }

    auto encode_container_name(const string_view name) -> string {
        string encoded;
        encoded.reserve(name.size());

        for (const auto ch : name) {
            if (ch == '/') {
                encoded += name_escape;
            } else {
                encoded += ch;
            }
        }

        return encoded;
    }

    auto load_weapon(istream &is) -> result<weapon_container> {
        if (!has_signature(is, weapon_signature)) {
            return make_error("not a weapon container");
        }

        weapon_container weapon;
        string line;

        while (getline(is, line)) {
            auto [key, value] = split_key(trim(line));

            if (key == "REALNAME") {
                weapon.name = value;
            } else if (key == "IMAGE") {
                weapon.image = value;
            } else if (key == "SCRIPT") {
                weapon.script = read_body(is, "SCRIPTEND");
            }
        }

        return weapon;
    }

    auto load_npc(istream &is) -> result<npc_container> {
        if (!has_signature(is, npc_signature)) {
            return make_error("not an npc container");
        }

        npc_container npc;
        string line;

        while (getline(is, line)) {
            auto [key, value] = split_key(trim(line));
            if (key.empty()) {
                continue;
            }

            if (key == "NPCSCRIPT") {
                npc.script = read_body(is, "NPCSCRIPTEND");
            } else if (key == "FLAG") {
                npc.flags.push_back(value);
            } else if (key == "JOINEDCLASSES") {
                npc.joined_classes = split(value, ',');
            } else if (key == "NAME") {
                npc.name = value;
            } else if (key == "IMAGE") {
                npc.image = value;
            } else {
                npc.attributes[key] = value;
            }
        }

        return npc;
    }

    auto load_weapon(const filesystem::path &path) -> result<weapon_container> {
        ifstream ifs(path, ios::binary);
        if (!ifs) {
            return make_error(format("could not open '{}'", path.string()));
        }

        auto weapon = load_weapon(ifs);
        if (weapon && weapon->name.empty()) {
            weapon->name = decode_container_name(path.stem().string());
        }

        return weapon;
    }

    auto load_npc(const filesystem::path &path) -> result<npc_container> {
        ifstream ifs(path, ios::binary);
        if (!ifs) {
            return make_error(format("could not open '{}'", path.string()));
        }

        auto npc = load_npc(ifs);
        if (npc && npc->name.empty()) {
            npc->name = decode_container_name(path.stem().string());
        }

        return npc;
    }
}
