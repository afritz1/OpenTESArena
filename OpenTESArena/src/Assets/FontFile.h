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
public:
	using Pixel = bool;
private:
	// One entry per character from ASCII 32 to 127 inclusive, with space (ASCII 32)
	// at index 0. Each letter's pixels are set (true) or unset (false).
	std::vector<Buffer2D<Pixel>> characters;
	int characterHeight;
public:
	bool init(const char *filename);

	int getWidth(char c) const;
	int getHeight() const;
	const Pixel *getPixels(char c) const;
};

#endif
