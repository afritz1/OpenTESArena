#ifndef SET_FILE_H
#define SET_FILE_H

#include <cstdint>

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer2D.h"

// A .SET file is packed with some uncompressed 64x64 wall .IMGs. Its size should
// be a multiple of 4096 bytes.

class SETFile
{
public:
	static constexpr int CHUNK_WIDTH = 64;
	static constexpr int CHUNK_HEIGHT = CHUNK_WIDTH;
private:
	Buffer<Buffer2D<uint8_t>> images;

	// Number of bytes in a 64x64 chunk (should be 4096).
	static constexpr int CHUNK_SIZE = SETFile::CHUNK_WIDTH * SETFile::CHUNK_HEIGHT;
public:
	bool init(const char *filename);

	// Gets the number of images.
	int getImageCount() const;

	// Gets the pixel data for a particular image of the .SET file.
	const uint8_t *getPixels(int index) const;
};

#endif
