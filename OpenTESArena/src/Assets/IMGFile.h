#ifndef IMG_FILE_H
#define IMG_FILE_H

#include <cstdint>
#include <memory>
#include <string>

#include "../Media/Palette.h"

// An IMG file can have one of a few formats; either with a header that determines
// properties, or without a header (either raw or a wall). Some IMGs also have a
// built-in palette, which they may or may not use eventually.

enum class PaletteName;

class IMGFile
{
private:
	int w, h;
	std::unique_ptr<uint32_t> pixels;
public:
	// Loads an IMG from file. Uses the given palette unless the palette name is
	// "built-in", then it refers to the IMG's palette instead.
	IMGFile(const std::string &filename, Palette *palette, PaletteName paletteName);
	~IMGFile();

	// Extracts the palette from an IMG file. Causes an exception if the IMG file 
	// doesn't have a palette.
	static Palette getPalette(const std::string &filename);

	int getWidth() const;
	int getHeight() const;
	uint32_t *getPixels() const;
};

#endif
