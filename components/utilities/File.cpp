#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>

#include "File.h"
#include "../debug/Debug.h"

std::string File::readAllText(const char *filename)
{
	std::ifstream ifs(filename, std::ios::in | std::ios::binary);

	DebugAssertMsg(ifs.is_open(), "Could not open \"" + std::string(filename) + "\".");

	ifs.seekg(0, std::ios::end);
	std::string text(ifs.tellg(), '\0');
	ifs.seekg(0, std::ios::beg);
	
	ifs.read(&text[0], text.size());
	
	return text;
}

bool File::exists(const char *filename)
{
	std::error_code code;
	bool success = std::filesystem::exists(filename, code);
	if (code)
	{
		DebugLogWarning("Couldn't determine if path \"" + std::string(filename) + "\" exists: " + code.message());
		return false;
	}

	if (!success)
	{
		// Path doesn't point to anything.
		return false;
	}

	success = std::filesystem::is_regular_file(filename, code);
	if (code)
	{
		DebugLog("Error determining if path \"" + std::string(filename) + "\" is a file: " + code.message());
		return false;
	}

	return success;
}

void File::copy(const char *srcFilename, const char *dstFilename)
{
	std::ifstream ifs(srcFilename, std::ios::binary);
	std::ofstream ofs(dstFilename, std::ios::binary);

	DebugAssertMsg(ifs.is_open(), "Cannot open \"" + std::string(srcFilename) + "\" for copying.");

	// Copy the source file to the destination.
	ofs << ifs.rdbuf();
}
