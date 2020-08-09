#ifndef FONT_FILE_H
#define FONT_FILE_H

#include <cstdint>
#include <vector>

#include "components/utilities/Buffer2D.h"

// This class loads a .DAT file containing font information. Each character is put
// into its own black and white image. White pixels are part of a character, while
// black pixels are part of the background (transparent).

class FontFile
{
private:
	// One entry per character from ASCII 32 to 127 inclusive, with space (ASCII 32)
	// at index 0. Each entry stores its letter as transparent and white pixel data.
	std::vector<Buffer2D<uint32_t>> characters;
	int characterHeight;
public:
	bool init(const char *filename);

	int getWidth(char c) const;
	int getHeight() const;
	const uint32_t *getPixels(char c) const;
};

#endif
