#ifndef DIRECTORY_H
#define DIRECTORY_H

namespace Directory
{
	bool exists(const char *path);

	// Creates a directory and all intermediate directories.
	void createRecursively(const char *path);
}

#endif
