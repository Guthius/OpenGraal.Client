#include "FileManager.h"

#include <raylib.h>
#include <boost/algorithm/string.hpp>

FileManager::FileMap FileManager::Files{};

void FileManager::BuildFileTable(const std::string &data_path)
{
	Files.clear();

	const auto path = std::filesystem::path(data_path);

	if (!exists(path))
	{
		TraceLog(LOG_FATAL, "FILEMANAGER: [%s] Data path does not exist!", data_path.c_str());

		return;
	}

	if (!is_directory(path))
	{
		TraceLog(LOG_FATAL, "FILEMANAGER: [%s] Data path is not a directory!", data_path.c_str());

		return;
	}

	for (const auto &entry: std::filesystem::recursive_directory_iterator(path))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}

		auto key = boost::to_lower_copy(entry.path().filename().string());

		Files[key] = entry.path();
	}

	TraceLog(LOG_INFO, "FILEMANAGER: File table loaded successfully (%d files)", Files.size());
}

auto FileManager::GetPath(const std::string &filename) -> std::filesystem::path
{
	if (filename.empty())
	{
		return {};
	}

	const auto key = boost::to_lower_copy(filename);
	const auto it = Files.find(key);

	if (it == Files.end())
	{
		TraceLog(LOG_WARNING, "FILEMANAGER: [%s] Could not locate file", filename.c_str());

		return {};
	}

	return it->second;
}
