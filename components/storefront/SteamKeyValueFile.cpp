#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string_view>

#include "SteamKeyValueFile.h"
#include "../debug/Debug.h"
#include "../utilities/String.h"
#include "../utilities/StringView.h"

namespace
{
	constexpr const char ACF_EXTENSION[] = "acf";
	constexpr const char VDF_EXTENSION[] = "vdf";
}

bool SteamKeyValueFile::init(const char *filename)
{
	if (String::isNullOrEmpty(filename))
	{
		DebugLogWarning("Missing/empty path for Steam key-value file parsing.");
		return false;
	}

	const std::string_view filenameExtension = StringView::getExtension(filename);
	if ((filenameExtension != ACF_EXTENSION) && (filenameExtension != VDF_EXTENSION))
	{
		DebugLogWarning("File \"" + std::string(filename) + "\" isn't a Steam key-value file.");
		return false;
	}

	// @todo: parse .vdf/.acf
	// return false if not a valid .vdf/.acf

	// return true if successful reading everything
	return true;
}
