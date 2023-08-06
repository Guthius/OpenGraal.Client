#include "FileManager.h"

#include <raylib.h>
#include <boost/algorithm/string.hpp>

FileManager::FileMap FileManager::Files{};

void FileManager::BuildFileTable(const std::string &dataPath)
{
	Files.clear();

	auto path = std::filesystem::path(dataPath);

	if (!exists(path))
	{
		TraceLog(LOG_FATAL, "FILEMANAGER: [%s] Data path does not exist!", dataPath.c_str());

		return;
	}

	if (!is_directory(path))
	{
		TraceLog(LOG_FATAL, "FILEMANAGER: [%s] Data path is not a directory!", dataPath.c_str());

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

std::filesystem::path FileManager::GetPath(const std::string &fileName)
{
	if (fileName.empty())
	{
		return {};
	}

	auto key = boost::to_lower_copy(fileName);
	auto it = Files.find(key);

	if (it == Files.end())
	{
		TraceLog(LOG_WARNING, "FILEMANAGER: [%s] Could not locate file", fileName.c_str());

		return {};
	}

	return it->second;
}