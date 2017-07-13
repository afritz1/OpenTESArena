#include <cctype>
#include <fstream>
#include <vector>

#include "SDL_platform.h"

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

bool File::pathIsRelative(const std::string &filename)
{
	DebugAssert(filename.size() > 0, "Path cannot be empty.");

	// See which platform we're running on by comparing with names provided by SDL.
	const std::string platformName(SDL_GetPlatform());

	if (platformName == "Windows")
	{
		// Can't be absolute without a colon at index 1.
		if (filename.size() < 2)
		{
			return true;
		}

		// Needs a drive letter and a colon to be absolute.
		return !(std::isalpha(filename.front()) && (filename.at(1) == ':'));
	}
	else
	{
		// Needs a leading forward slash to be absolute.
		return filename.front() != '/';
	}
}

void File::copy(const std::string &srcFilename, const std::string &dstFilename)
{
	std::ifstream ifs(srcFilename, std::ios::binary);
	std::ofstream ofs(dstFilename, std::ios::binary);

	DebugAssert(ifs.is_open(), "Cannot open \"" + srcFilename + "\" for copying.");

	// Copy the source file to the destination.
	ofs << ifs.rdbuf();

	ifs.close();
	ofs.close();
}
