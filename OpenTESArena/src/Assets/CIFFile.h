#ifndef CIF_FILE_H
#define CIF_FILE_H

#include <cstdint>
#include <vector>

#include "../Math/Vector2.h"

#include "components/utilities/Buffer2D.h"

// A .CIF file has one or more images, and each image has some frames associated
// with it. Examples of .CIF images are character faces, cursors, and weapon 
// animations.

class CIFFile
{
private:
	std::vector<Buffer2D<uint8_t>> images;
	std::vector<Int2> offsets;
public:
	bool init(const char *filename);

	// Gets the number of images.
	int getImageCount() const;

	// Gets the X offset from the left screen edge in pixels.
	int getXOffset(int index) const;

	// Gets the Y offset from the top screen edge in pixels.
	int getYOffset(int index) const;

	// Gets the width of an image.
	int getWidth(int index) const;

	// Gets the height of an image.
	int getHeight(int index) const;

	// Gets a pointer to an image's 8-bit pixels.
	const uint8_t *getPixels(int index) const;
};

#endif
