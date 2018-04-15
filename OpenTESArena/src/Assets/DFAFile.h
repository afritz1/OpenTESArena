#ifndef DFA_FILE_H
#define DFA_FILE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// A DFA file contains images for entities that animate but don't move in the world, 
// like shopkeepers, tavern folk, lamps, fountains, staff pieces, and torches.

class DFAFile
{
private:
	std::vector<std::unique_ptr<uint8_t[]>> pixels;
	int width, height;
public:
	DFAFile(const std::string &filename);

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
