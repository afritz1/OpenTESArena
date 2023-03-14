#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <string>

namespace Directory
{
	bool exists(const std::string &path);

	// Creates a directory and all intermediate directories.
	void createRecursively(std::string path);
}

#endif
