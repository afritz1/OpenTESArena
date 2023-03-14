#include <filesystem>

#include "Directory.h"

#include "../debug/Debug.h"

bool Directory::exists(const char *path)
{
	std::error_code code;
	bool success = std::filesystem::exists(path, code);
	if (code)
	{
		DebugLogWarning("Couldn't determine if path \"" + std::string(path) + "\" exists: " + code.message());
		return false;
	}

	if (!success)
	{
		// Path doesn't point to anything.
		return false;
	}
	
	success = std::filesystem::is_directory(path, code);
	if (code)
	{
		DebugLog("Error determining if path \"" + std::string(path) + "\" is a directory: " + code.message());
		return false;
	}

	return success;
}

void Directory::createRecursively(const char *path)
{
	std::error_code code;
	const bool success = std::filesystem::create_directories(path, code);
	if (code)
	{
		DebugLog("Error creating directories for \"" + std::string(path) + "\": " + code.message());
	}
	else if (!success)
	{
		DebugLogWarning("Couldn't create directories for \"" + std::string(path) + "\".");
	}
}
