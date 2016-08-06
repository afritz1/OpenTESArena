#include <unordered_map>

#include "CIFFile.h"

#include "../Math/Int2.h"
#include "../Utilities/Debug.h"

namespace
{
	// These CIF files are headerless with hardcoded dimensions.
	const std::unordered_map<std::string, Int2> RawCifOverride =
	{
		{ "BRASS.CIF", { 8, 8 } },
		{ "BRASS2.CIF", { 8, 8 } },
		{ "MARBLE.CIF", { 3, 3 } },
		{ "MARBLE2.CIF", { 3, 3 } },
		{ "PARCH.CIF", { 20, 20 } },
		{ "SCROLL.CIF", { 20, 20 } }
	};
}

CIFFile::CIFFile(const std::string &filename)
{
	Debug::crash("CIFFile", "Not implemented.");
}

CIFFile::~CIFFile()
{

}

/*std::vector<SDL_Surface*> TextureManager::loadCIF(const std::string &filename,
	PaletteName paletteName)
{
	// CIF is for player weapon animations, some arrow cursors, and character heads.

	Debug::crash("Texture Manager", "loadCIF not implemented.");
	return std::vector<SDL_Surface*>();
}*/
