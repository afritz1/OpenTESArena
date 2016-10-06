#ifndef DFA_FILE_H
#define DFA_FILE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Media/Palette.h"

// A DFA file contains animation frames for static entities, like the bartender
// and various kinds of torches.

class DFAFile
{
private:
	std::vector<std::unique_ptr<uint32_t>> pixels;
	int width, height;
public:
	DFAFile(const std::string &filename, const Palette &palette);
	~DFAFile();

	// Gets the number of images in the DFA file.
	int getImageCount() const;

	// Gets the width of an image in the DFA file.
	int getWidth() const;

	// Gets the height of an image in the DFA file.
	int getHeight() const;

	// Gets a pointer to the pixels for an image in the DFA file.
	uint32_t *getPixels(int index) const;
};

#endif
