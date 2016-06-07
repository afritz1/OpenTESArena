#include <fstream>
#include <vector>

#include "File.h"

#include "Debug.h"

std::string File::toString(const std::string &filename)
{
	/*
	auto ifs = std::ifstream(filename.c_str(), std::ios::in |
		std::ios::binary | std::ios::ate);

	Debug::check(ifs.is_open(), "File", "Could not open file \"" + filename + "\".");

	auto fileSize = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	auto bytes = std::vector<char>(fileSize);
	ifs.read(&bytes.at(0), fileSize);
	ifs.close();

	return std::string(&bytes.at(0), fileSize);
	*/
}
