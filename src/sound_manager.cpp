#include "sound_manager.hpp"

#include "file_manager.hpp"

#include <map>
#include <boost/algorithm/string.hpp>

namespace {
    std::map<std::string, Sound> loaded_sounds;

    auto load_sound_from_file(const std::string &key) -> Sound {
        const auto path = find_file(key);

        if (path.empty()) {
            loaded_sounds[key] = {};

            return {};
        }

        const auto sound = LoadSound(path.string().c_str());

        loaded_sounds[key] = sound;

        return sound;
    }
}

auto load_sound(const std::string &filename) -> Sound {
    const auto key = boost::to_lower_copy(filename);
    const auto iter = loaded_sounds.find(key);

    if (iter != loaded_sounds.end()) {
        return iter->second;
    }

    return load_sound_from_file(key);
}

void play_sound(const std::string &filename) {
    if (const auto sound = load_sound(filename); IsSoundValid(sound)) {
        PlaySound(sound);
    }
}
