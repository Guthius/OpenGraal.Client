#include "file_manager.hpp"

#include <fstream>
#include <map>
#include <raylib.h>
#include <shared/text.hpp>

namespace {
    std::map<std::string, std::filesystem::path> file_map{};
}

void build_file_table(const std::string &data_path) {
    file_map.clear();

    const auto path = std::filesystem::path(data_path);

    if (!exists(path)) {
        TraceLog(LOG_FATAL, "FILEMANAGER: [%s] Data path does not exist!", data_path.c_str());

        return;
    }

    if (!is_directory(path)) {
        TraceLog(LOG_FATAL, "FILEMANAGER: [%s] Data path is not a directory!", data_path.c_str());

        return;
    }

    for (const auto &entry : std::filesystem::recursive_directory_iterator(path)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        auto key = og::shared::to_lower(entry.path().filename().string());

        file_map[key] = entry.path();
    }

    TraceLog(LOG_INFO, "FILEMANAGER: File table loaded successfully (%d files)", file_map.size());
}

auto find_file(const std::string &filename) -> std::filesystem::path {
    if (filename.empty()) {
        return {};
    }

    const auto key = og::shared::to_lower(filename);
    const auto it = file_map.find(key);

    if (it == file_map.end()) {
        TraceLog(LOG_WARNING, "FILEMANAGER: [%s] Could not locate file", filename.c_str());

        return {};
    }

    return it->second;
}

auto has_file(const std::string &filename) -> bool {
    return !filename.empty() && file_map.contains(og::shared::to_lower(filename));
}

auto store_file(const std::string &filename, const std::string &data) -> bool {
    const auto name = std::filesystem::path(filename).filename();
    if (name.empty()) {
        return false;
    }

    std::error_code failed;
    std::filesystem::create_directories("downloads", failed);

    const auto path = std::filesystem::path("downloads") / name;

    std::ofstream os(path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!os.is_open()) {
        return false;
    }

    os << data;
    if (!os.good()) {
        return false;
    }

    os.close();
    file_map[og::shared::to_lower(name.string())] = path;

    return true;
}
