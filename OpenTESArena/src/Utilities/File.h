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
};

#endif
