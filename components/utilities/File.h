#ifndef FILE_H
#define FILE_H

#include <cstddef>
#include <string>

#include "Buffer.h"

namespace File
{
	std::string readAllText(const char *filename);
	Buffer<std::byte> readAllBytes(const char *filename);

	bool exists(const char *filename);

	void copy(const char *srcFilename, const char *dstFilename);
}

#endif
