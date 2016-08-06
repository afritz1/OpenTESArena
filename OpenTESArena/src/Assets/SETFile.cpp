#include "SETFile.h"

#include "../Utilities/Debug.h"

SETFile::SETFile(const std::string &filename)
{
	Debug::crash("SETFile", "Not implemented.");
}

SETFile::~SETFile()
{

}

/*std::vector<SDL_Surface*> TextureManager::loadSET(const std::string &filename,
	PaletteName paletteName)
{
	// A SET is just some IMGs packed together vertically, so split them here
	// and return them separately in a vector. They're basically all uncompressed
	// wall textures grouped by type.

	// The height of the image should be a multiple of 64, so divide by 64 to get
	// the number of IMGs in the SET. Alternatively, since all wall textures are
	// 4096 bytes, just divide the SET byte size by 4096 for the IMG count.

	Debug::crash("Texture Manager", "loadSET not implemented.");
	return std::vector<SDL_Surface*>();
}*/
