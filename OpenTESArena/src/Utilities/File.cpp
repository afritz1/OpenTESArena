#include <cctype>
#include <fstream>
#include <vector>

#include "Debug.h"
#include "File.h"
#include "Platform.h"

std::string File::readAllText(const std::string &filename)
{
	std::ifstream ifs(filename, std::ios::in | std::ios::binary);

	DebugAssertMsg(ifs.is_open(), "Could not open \"" + filename + "\".");

	ifs.seekg(0, std::ios::end);
	std::string text(ifs.tellg(), '\0');
	ifs.seekg(0, std::ios::beg);
	
	ifs.read(&text[0], text.size());
	
	return text;
}

bool File::exists(const std::string &filename)
{
	std::ifstream ifs(filename);
	const bool isOpen = ifs.good();
	return isOpen;
}

bool File::pathIsRelative(const std::string &filename)
{
	DebugAssertMsg(filename.size() > 0, "Path cannot be empty.");

	// See which platform we're running on by comparing with names provided by SDL.
	const std::string platformName = Platform::getPlatform();

	if (platformName == "Windows")
	{
		// Can't be absolute without a colon at index 1.
		if (filename.size() < 2)
		{
			return true;
		}

		// Needs a drive letter and a colon to be absolute.
		return !(std::isalpha(static_cast<unsigned char>(filename.front())) && (filename.at(1) == ':'));
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

	DebugAssertMsg(ifs.is_open(), "Cannot open \"" + srcFilename + "\" for copying.");

	// Copy the source file to the destination.
	ofs << ifs.rdbuf();
}
