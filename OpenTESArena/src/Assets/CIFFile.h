#ifndef CIF_FILE_H
#define CIF_FILE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Math/Vector2.h"

// A CIF file has one or more images, and each image has some frames associated
// with it. Examples of CIF images are character faces, cursors, and weapon 
// animations.

class CIFFile
{
private:
	std::vector<std::unique_ptr<uint8_t[]>> pixels;
	std::vector<Int2> offsets, dimensions;
public:
	CIFFile(const std::string &filename);

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
