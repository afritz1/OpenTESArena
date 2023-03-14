#ifndef FILE_H
#define FILE_H

#include <string>

namespace File
{
	// Reads all of a file's text into a string.
	std::string readAllText(const char *filename);

	// Checks that a file exists.
	bool exists(const char *filename);

	// Copies a file to a destination file.
	void copy(const char *srcFilename, const char *dstFilename);
}

#endif
