#include <algorithm>
#include <filesystem>

#include "Directory.h"
#include "StringView.h"

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

int Directory::getFileCount(const char *path)
{
	if (!Directory::exists(path))
	{
		return 0;
	}

	const std::filesystem::directory_iterator dirIter(path);
	const std::filesystem::directory_iterator dirIterEnd;
	const size_t count = std::count_if(dirIter, dirIterEnd,
		[](const std::filesystem::directory_entry &entry)
	{
		std::error_code dummy;
		return entry.is_regular_file(dummy);
	});

	return static_cast<int>(count);
}

std::vector<std::string> Directory::getFilesWithExtension(const char *path, const char *extension)
{
	std::vector<std::string> filenames;
	if (!Directory::exists(path))
	{
		return filenames;
	}

	const std::filesystem::path fsPath(path);
	for (const std::filesystem::directory_entry &dirEntry : std::filesystem::directory_iterator(fsPath))
	{
		std::error_code code;
		if (!dirEntry.is_regular_file(code))
		{
			continue;
		}

		const std::filesystem::path dirEntryPath = dirEntry.path();
		const std::string dirEntryExtension = dirEntryPath.extension().string();
		if (!StringView::caseInsensitiveEquals(dirEntryExtension, extension))
		{
			continue;
		}

		filenames.emplace_back(dirEntryPath.string());
	}

	std::sort(filenames.begin(), filenames.end());
	return filenames;
}

void Directory::deleteOldestFile(const char *path)
{
	if (!Directory::exists(path))
	{
		return;
	}

	std::error_code code;
	std::filesystem::file_time_type oldestTime = std::filesystem::file_time_type::max();
	std::filesystem::path oldestFilePath;
	for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(path))
	{
		const std::filesystem::path &currentFilePath = entry.path();
		const std::filesystem::file_time_type currentTime = entry.last_write_time(code);
		if (code)
		{
			DebugLog("Error getting last write time of file \"" + currentFilePath.string() + "\": " + code.message());
			continue;
		}

		if (currentTime < oldestTime)
		{
			oldestTime = currentTime;
			oldestFilePath = currentFilePath;
		}
	}

	if (!oldestFilePath.empty())
	{
		std::filesystem::remove(oldestFilePath, code);
		if (code)
		{
			DebugLog("Error deleting oldest file \"" + oldestFilePath.string() + "\": " + code.message());
		}
	}
}
