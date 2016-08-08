#ifndef PALETTE_FILE_H
#define PALETTE_FILE_H

#include <string>

// Similar to "TextureFile", but for getting the filename of a palette, given
// the PaletteName.

enum class PaletteName;

class PaletteFile
{
private:
	PaletteFile() = delete;
	PaletteFile(const PaletteFile&) = delete;
	~PaletteFile() = delete;
public:
	static const std::string &fromName(PaletteName paletteName);
};

#endif
