#ifndef CIF_FILE_H
#define CIF_FILE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Math/Vector2.h"
#include "../Media/Palette.h"

// A CIF file has one or more images, and each image has some frames associated
// with it. Examples of CIF images are character faces, cursors, and weapon 
// animations.

class CIFFile
{
private:
	std::vector<std::unique_ptr<uint32_t[]>> pixels;
	std::vector<Int2> offsets;
	std::vector<Int2> dimensions;
public:
	CIFFile(const std::string &filename, const Palette &palette);
	~CIFFile();

	// Gets the number of images in the CIF file.
	int getImageCount() const;

	// Gets the X offset from the left screen edge in pixels.
	int getXOffset(int index) const;

	// Gets the Y offset from the top screen edge in pixels.
	int getYOffset(int index) const;

	// Gets the width of an image in the CIF file.
	int getWidth(int index) const;

	// Gets the height of an image in the CIF file.
	int getHeight(int index) const;

	// Gets a pointer to the pixels for an image in the CIF file.
	uint32_t *getPixels(int index) const;
};

#endif
