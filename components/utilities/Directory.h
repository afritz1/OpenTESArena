#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <string>
#include <vector>

// Filesystem functions.
namespace Directory
{
	bool exists(const char *path);

	// Creates a directory and all intermediate directories.
	void createRecursively(const char *path);

	// Counts the number of regular files in the given directory.
	int getFileCount(const char *path);

	// Returns filenames in the directory matching the extension.
	std::vector<std::string> getFilesWithExtension(const char *path, const char *extension);

	void deleteOldestFile(const char *path);
}

#endif
