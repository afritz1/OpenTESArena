#ifndef FONT_FILE_H
#define FONT_FILE_H

#include <cstdint>

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer2D.h"
#include "components/utilities/BufferView2D.h"

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
	Buffer<Buffer2D<Pixel>> characters;
	int characterHeight;
public:
	bool init(const char *filename);

	// Attempts to convert an ASCII character to its index in the characters list.
	static bool tryGetCharacterIndex(char c, int *outIndex);

	// Attempts to convert a character index to its associated ASCII character.
	static bool tryGetChar(int index, char *outChar);

	int getCharacterCount() const;
	int getHeight() const;
	BufferView2D<const Pixel> getPixels(int index) const;
};

#endif
