#pragma once

#include <map>
#include <string>
#include <filesystem>

class FileManager
{
	using FileKey = std::string;
	using FilePath = std::filesystem::path;
	using FileMap = std::map<FileKey, FilePath>;

public:
	static void BuildFileTable(const std::string &data_path);

	static auto GetPath(const std::string &filename) -> std::filesystem::path;

private:
	static FileMap Files;
};
