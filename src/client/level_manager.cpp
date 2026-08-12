#include "level_manager.hpp"

#include "file_manager.hpp"

#include <map>
#include <shared/text.hpp>

namespace {
    std::map<std::string, std::shared_ptr<level>> loaded_levels;

    auto load_level_from_file(const std::string &key) -> std::shared_ptr<level> {
        const auto path = find_file(key);

        if (path.empty()) {
            TraceLog(LOG_ERROR, "Could not find level '%s'", key.c_str());

            loaded_levels[key] = nullptr;

            return nullptr;
        }

        const auto level = level::load(path);

        loaded_levels[key] = level;

        if (level == nullptr) {
            TraceLog(LOG_ERROR, "Failed to load level '%s'", key.c_str());
        }

        return level;
    }
}

auto load_level(const std::string &name) -> std::shared_ptr<level> {
    const auto key = og::shared::to_lower(name);
    const auto iter = loaded_levels.find(key);

    if (iter == loaded_levels.end()) {
        return load_level_from_file(key);
    }

    return iter->second;
}

void install_level(const std::string &name, const std::shared_ptr<level> &value) {
    loaded_levels[og::shared::to_lower(name)] = value;
}

auto has_level(const std::string &name) -> bool {
    const auto match = loaded_levels.find(og::shared::to_lower(name));

    return match != loaded_levels.end() && match->second != nullptr;
}
