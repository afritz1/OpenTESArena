#ifndef RCI_FILE_H
#define RCI_FILE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// An RCI file is for screen-space animations like water and lava. It is packed 
// with five uncompressed 320x100 images.

class RCIFile
{
private:
	// One unique_ptr for each frame.
	std::vector<std::unique_ptr<uint8_t[]>> pixels;

	// Number of bytes in a 320x100 frame (should be 32000).
	static const int FRAME_SIZE;
public:
	RCIFile(const std::string &filename);

	// All individual frames are 320x100.
	static const int WIDTH;
	static const int HEIGHT;

	// Gets the number of frames (should be 5).
	int getImageCount() const;

	// Gets the pixel data for a 320x100 frame.
	const uint8_t *getPixels(int index) const;
};

#endif
