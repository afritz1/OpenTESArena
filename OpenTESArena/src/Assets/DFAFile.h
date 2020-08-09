#ifndef DFA_FILE_H
#define DFA_FILE_H

#include <cstdint>

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer2D.h"

// A .DFA file contains images for entities that animate but don't move in the world, 
// like shopkeepers, tavern folk, lamps, fountains, staff pieces, and torches.

class DFAFile
{
private:
	Buffer<Buffer2D<uint8_t>> images;
	int width, height;
public:
	bool init(const char *filename);

	// Gets the number of images.
	int getImageCount() const;

	// Gets the width of all images.
	int getWidth() const;

	// Gets the height of all images.
	int getHeight() const;

	// Gets a pointer to an image's 8-bit pixels.
	const uint8_t *getPixels(int index) const;
};

#endif
