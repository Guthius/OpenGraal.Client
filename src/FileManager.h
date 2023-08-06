#pragma once

#include <map>
#include <string>
#include <filesystem>

class FileManager
{
private:
	typedef std::string FileKey;
	typedef std::filesystem::path FilePath;
	typedef std::map<FileKey, FilePath> FileMap;

public:
	static void BuildFileTable(const std::string &dataPath);

	static std::filesystem::path GetPath(const std::string &fileName);

private:
	static FileMap Files;
};