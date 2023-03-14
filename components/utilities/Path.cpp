#include <filesystem>

#include "Path.h"

bool Path::isAbsolute(const char *path)
{
	const std::filesystem::path fsPath(path);
	return fsPath.is_absolute();
}

bool Path::isRelative(const char *path)
{
	const std::filesystem::path fsPath(path);
	return fsPath.is_relative();
}
