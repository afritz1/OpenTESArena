#ifndef FONT_FILE_H
#define FONT_FILE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// This class loads a .DAT file containing font information. Each character is put
// into its own black and white image. White pixels are part of a character, while
// black pixels are part of the background (transparent).

class FontFile
{
private:
	// One entry per character from ASCII 32 to 127 inclusive, with space (ASCII 32)
	// at index 0. Each pair has the width of its letter in pixels with its black 
	// and white pixel data.
	std::vector<std::pair<int, std::unique_ptr<uint32_t[]>>> characters;
	int characterHeight;
public:
	FontFile(const std::string &filename);
	~FontFile();

	int getWidth(char c) const;
	int getHeight() const;
	uint32_t *getPixels(char c) const;
};

#endif
