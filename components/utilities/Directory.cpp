#include <filesystem>

#include "Directory.h"

#include "String.h"
#include "../debug/Debug.h"

bool Directory::exists(const char *path)
{
	if (String::isNullOrEmpty(path))
	{
		DebugLogWarning("Can't check if empty path exists.");
		return false;
	}

	return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

void Directory::createRecursively(const char *path)
{
	if (String::isNullOrEmpty(path))
	{
		DebugLogWarning("Can't create directories from empty path.");
		return;
	}

	const bool success = std::filesystem::create_directories(path);
	if (!success)
	{
		DebugLogWarning("Couldn't create one or more directories from \"" + std::string(path) + "\".");
	}
}