#ifndef FILE_H
#define FILE_H

#include <string>

class File
{
private:
	File() = delete;
	File(const File&) = delete;
	~File() = delete;
public:
	// Reads a file into a string.
	static std::string toString(const std::string &filename);

	// Checks that a file exists.
	static bool exists(const std::string &filename);

	// Checks if the path to a file is relative or absolute.
	static bool pathIsRelative(const std::string &filename);
};

#endif
