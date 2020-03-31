#ifndef PALETTE_FILE_H
#define PALETTE_FILE_H

#include <string>

// Similar to "TextureFile", but for getting the filename of a palette, given
// the PaletteName.

enum class PaletteName;

namespace PaletteFile
{
	const std::string &fromName(PaletteName paletteName);
}

#endif
