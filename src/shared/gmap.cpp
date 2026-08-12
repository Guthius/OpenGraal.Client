#include <shared/gmap.hpp>

#include <shared/text.hpp>

#include <format>
#include <fstream>

using namespace std;

namespace og::shared {
    namespace {
        constexpr string_view gmap_signature = "GRMAP001";

        auto strip_quotes(const string_view str) -> string {
            auto text = trim(str);

            if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
                return text.substr(1, text.size() - 2);
            }

            return text;
        }

        auto read_list(istream &is, const string_view terminator) -> vector<string> {
            vector<string> values;
            string line;

            while (getline(is, line)) {
                if (trim(line) == terminator) {
                    break;
                }

                values.push_back(line);
            }

            return values;
        }

        void read_level_names(gmap_data &gmap, istream &is) {
            for (const auto &row : read_list(is, "LEVELNAMESEND")) {
                for (const auto &entry : split(row, ',')) {
                    gmap.level_names.push_back(strip_quotes(entry));
                }
            }
        }

        void read_height_map(gmap_data &gmap, istream &is) {
            for (const auto &row : read_list(is, "HEIGHTMAPEND")) {
                for (const auto &entry : split(row, ',')) {
                    gmap.height_map.push_back(to_int(entry));
                }
            }
        }

        void read_random_seeds(gmap_data &gmap, istream &is) {
            for (const auto &row : read_list(is, "RANDOMSEEDSEND")) {
                for (const auto &entry : split(row, ',')) {
                    gmap.random_seeds.push_back(static_cast<uint32_t>(strtoul(string(trim(entry)).c_str(), nullptr, 10)));
                }
            }
        }

        auto column_name(int index) -> string {
            return {static_cast<char>('a' + (index / 26)), static_cast<char>('a' + (index % 26))};
        }
    }

    auto gmap_data::level_at(const int x, const int y) const -> string {
        if (x < 0 || y < 0 || x >= width || y >= height) {
            return {};
        }

        const auto index = (static_cast<size_t>(y) * static_cast<size_t>(width)) + static_cast<size_t>(x);

        return index < level_names.size() ? level_names[index] : string{};
    }

    auto gmap_data::position_of(const string_view level_name) const -> optional<gmap_position> {
        for (size_t i = 0; i < level_names.size(); ++i) {
            if (iequals(level_names[i], level_name)) {
                return gmap_position{
                    .x = static_cast<int>(i % static_cast<size_t>(width)),
                    .y = static_cast<int>(i / static_cast<size_t>(width)),
                };
            }
        }

        return nullopt;
    }

    auto derive_generated_level_names(const string_view generated, const int width, const int height) -> vector<string> {
        const auto separator = generated.rfind('_');
        if (separator == string_view::npos || width <= 0 || height <= 0) {
            return {};
        }

        const auto prefix = string(generated.substr(0, separator + 1));
        const auto extension = filesystem::path(generated).extension().string();

        vector<string> names;
        names.reserve(static_cast<size_t>(width) * static_cast<size_t>(height));

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                names.push_back(format("{}{}-{:02}{}", prefix, column_name(x), y + 1, extension));
            }
        }

        return names;
    }

    auto load_gmap(string name, istream &is) -> result<gmap_data> {
        auto gmap = gmap_data{.name = std::move(name)};

        string line;
        if (!getline(is, line) || trim(line) != gmap_signature) {
            return make_error(format("'{}' is not a gmap", gmap.name));
        }

        while (getline(is, line)) {
            const auto trimmed = trim(line);
            if (trimmed.empty()) {
                continue;
            }

            const auto separator = trimmed.find(' ');
            const auto key = trimmed.substr(0, separator);
            const auto value = separator == string::npos ? string{} : trim(trimmed.substr(separator + 1));

            if (key == "WIDTH") {
                gmap.width = to_int(value);
            } else if (key == "HEIGHT") {
                gmap.height = to_int(value);
            } else if (key == "GENERATED") {
                gmap.generated = value;
            } else if (key == "MAPIMG") {
                gmap.map_image = value;
            } else if (key == "MINIMAPIMG") {
                gmap.minimap_image = value;
            } else if (key == "NOAUTOMAPPING") {
                gmap.no_auto_mapping = true;
            } else if (key == "LOADFULLMAP") {
                gmap.load_full_map = true;
            } else if (key == "LEVELNAMES") {
                read_level_names(gmap, is);
            } else if (key == "HEIGHTMAP") {
                read_height_map(gmap, is);
            } else if (key == "LOADATSTART") {
                gmap.load_at_start = read_list(is, "LOADATSTARTEND");
            } else if (key == "LEVHEIGHT") {
                gmap.level_height = to_float(value);
            } else if (key == "LEVCHAOS") {
                gmap.level_chaos = to_float(value);
            } else if (key == "RANDOMSEEDS") {
                read_random_seeds(gmap, is);
            }
        }

        if (gmap.level_names.empty() && !gmap.generated.empty()) {
            gmap.level_names = derive_generated_level_names(gmap.generated, gmap.width, gmap.height);
        }

        return gmap;
    }

    auto load_gmap(const filesystem::path &path) -> result<gmap_data> {
        if (!is_regular_file(path)) {
            return make_error(format("'{}' is not a file", path.string()));
        }

        ifstream ifs(path);
        if (!ifs) {
            return make_error(format("could not open '{}'", path.string()));
        }

        return load_gmap(path.filename().string(), ifs);
    }
}
