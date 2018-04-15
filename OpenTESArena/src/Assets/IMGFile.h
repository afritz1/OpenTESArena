#ifndef IMG_FILE_H
#define IMG_FILE_H

#include <cstdint>
#include <memory>
#include <string>

#include "../Media/Palette.h"

// An IMG file can have one of a few formats; either with a header that determines
// properties, or without a header (either raw or a wall). Some IMGs also have a
// built-in palette, which they may or may not use eventually.

class IMGFile
{
private:
	std::unique_ptr<uint8_t[]> pixels;
	std::unique_ptr<Palette> palette;
	int width, height;

	// Reads the palette from an .IMG file's palette data.
	static Palette readPalette(const uint8_t *paletteData);
public:
	IMGFile(const std::string &filename);

	// Extracts the palette from an .IMG file. Causes an error if the .IMG doesn't
	// have a palette.
	static Palette extractPalette(const std::string &filename);

	// Gets the width in pixels.
	int getWidth() const;

	// Gets the height in pixels.
	int getHeight() const;

	// Gets the image's palette, or null if it doesn't have one.
	const Palette *getPalette() const;

	// Gets a pointer to the image's pixels.
	const uint8_t *getPixels() const;
};

#endif
