#ifndef RCI_FILE_H
#define RCI_FILE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Media/Palette.h"

// An RCI file is for screen-space animations like water and lava. It is packed 
// with five uncompressed 320x100 images.

class RCIFile
{
private:
	// One unique_ptr for each frame of the RCI.
	std::vector<std::unique_ptr<uint32_t>> frames;

	// Number of bytes in a 320x100 frame (should be 32000).
	static const int FRAME_SIZE;
public:
	RCIFile(const std::string &filename, const Palette &palette);
	~RCIFile();

	// All individual frames of an RCI are 320x100.
	static const int FRAME_WIDTH;
	static const int FRAME_HEIGHT;

	// Gets the number of frames in the RCI (should be 5).
	int getCount() const;

	// Gets the pixel data for a 320x100 frame of an RCI file.
	uint32_t *getPixels(int index) const;
};

#endif
