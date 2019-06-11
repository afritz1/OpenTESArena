#ifndef SET_FILE_H
#define SET_FILE_H

#include <cstdint>
#include <memory>
#include <vector>

// A SET file is packed with some uncompressed 64x64 wall IMGs. Its size should
// be a multiple of 4096 bytes.

class SETFile
{
private:
	// One unique_ptr for each 64x64 image.
	std::vector<std::unique_ptr<uint8_t[]>> pixels;

	// Number of bytes in a 64x64 chunk (should be 4096).
	static const int CHUNK_SIZE;
public:
	bool init(const char *filename);

	// All individual images (chunks) are 64x64.
	static const int CHUNK_WIDTH;
	static const int CHUNK_HEIGHT;

	// Gets the number of images.
	int getImageCount() const;

	// Gets the pixel data for a 64x64 chunk.
	const uint8_t *getPixels(int index) const;
};

#endif
