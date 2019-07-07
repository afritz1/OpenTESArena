#ifndef FILE_H
#define FILE_H

#include <string>

class File
{
private:
	File() = delete;
	~File() = delete;
public:
	// Reads all of a file's text into a string.
	static std::string readAllText(const char *filename);

	// Checks that a file exists.
	static bool exists(const char *filename);

	// Checks if the path to a file is relative or absolute.
	static bool pathIsRelative(const char *filename);

	// Copies a file to a destination file.
	static void copy(const char *srcFilename, const char *dstFilename);
};

#endif
