#include "animation_manager.hpp"

#include "file_manager.hpp"
#include "network.hpp"

#include <map>
#include <shared/text.hpp>

namespace {
    std::map<std::string, animation *> loaded_animations;

    auto load_animation_from_file(const std::filesystem::path &path) -> animation * {
        auto *const animation = new ::animation();

        animation->load_file(path);

        const auto key = og::shared::to_lower(path.stem().string());

        return loaded_animations[key] = animation;
    }
}

auto load_animation(const std::string &name) -> animation * {
    const auto key = og::shared::to_lower(name);
    const auto iter = loaded_animations.find(key);

    if (iter != loaded_animations.end()) {
        return iter->second;
    }

    const auto filename = key + ".gani";
    if (!has_file(filename)) {
        get_network().request_file(filename);

        return loaded_animations[key] = nullptr;
    }

    return load_animation_from_file(find_file(filename));
}

void load_animations_from_directory(const std::filesystem::path &path) {
    if (!is_directory(path)) {
        return;
    }

    for (const auto &file : std::filesystem::directory_iterator(path)) {
        if (!file.is_regular_file()) {
            continue;
        }

        if (auto ext = og::shared::to_lower(file.path().extension().string()); ext != ".gani") {
            continue;
        }

        load_animation_from_file(file.path());
    }
}

void forget_animation(const std::string &name) {
    loaded_animations.erase(og::shared::to_lower(std::filesystem::path(name).stem().string()));
}
