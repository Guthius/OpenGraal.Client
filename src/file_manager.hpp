#pragma once

#include <filesystem>
#include <string>

void build_file_table(const std::string &data_path);
auto find_file(const std::string &filename) -> std::filesystem::path;
