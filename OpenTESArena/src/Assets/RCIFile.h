#ifndef RCI_FILE_H
#define RCI_FILE_H

#include <cstdint>

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer2D.h"

// An .RCI file is for screen-space animations like water and lava. It is packed 
// with five uncompressed 320x100 images.

class RCIFile
{
public:
	static constexpr int WIDTH = 320;
	static constexpr int HEIGHT = 100;
private:
	Buffer<Buffer2D<uint8_t>> images;

	// Number of bytes in a 320x100 frame (should be 32000).
	static constexpr int FRAME_SIZE = RCIFile::WIDTH * RCIFile::HEIGHT;
public:
	bool init(const char *filename);

	// Gets the number of frames (should be 5).
	int getImageCount() const;

	// Gets the pixel data for a 320x100 frame.
	const uint8_t *getPixels(int index) const;
};

#endif
