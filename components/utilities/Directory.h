#ifndef DIRECTORY_H
#define DIRECTORY_H

// Filesystem functions.
namespace Directory
{
	bool exists(const char *path);

	// Creates a directory and all intermediate directories.
	void createRecursively(const char *path);

	// Counts the number of regular files in the given directory.
	int getFileCount(const char *path);

	void deleteOldestFile(const char *path);
}

#endif
