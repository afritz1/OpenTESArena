#include <fstream>
#include <vector>

#include "File.h"

#include "Debug.h"

std::string File::toString(const std::string &filename)
{
	std::ifstream ifs(filename.c_str(), std::ios::in |
		std::ios::binary | std::ios::ate);

	DebugAssert(ifs.is_open(), "Could not open \"" + filename + "\".");

	const auto fileSize = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	std::vector<char> bytes(fileSize);
	ifs.read(bytes.data(), fileSize);
	ifs.close();

	return std::string(bytes.data(), fileSize);
}

bool File::exists(const std::string &filename)
{
	std::ifstream ifs(filename);
	const bool isOpen = ifs.good();

	ifs.close();
	return isOpen;
}
