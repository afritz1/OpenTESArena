#include <fstream>
#include <vector>

#include "File.h"

#include "Debug.h"

std::string File::toString(const std::string &filename)
{
	std::ifstream ifs(filename.c_str(), std::ios::in |
		std::ios::binary | std::ios::ate);

	Debug::check(ifs.is_open(), "File", "Could not open \"" + filename + "\".");

	auto fileSize = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	std::vector<char> bytes(fileSize);
	ifs.read(&bytes.at(0), fileSize);
	ifs.close();

	return std::string(&bytes.at(0), fileSize);
}
