#ifndef SET_FILE_H
#define SET_FILE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Media/Palette.h"

// A SET file is packed with some uncompressed 64x64 wall IMGs. Its size should
// be a multiple of 4096 bytes.

class SETFile
{
private:
	// One unique_ptr for each 64x64 chunk of the SET.
	std::vector<std::unique_ptr<uint32_t>> chunks;

	// Number of bytes in a 64x64 chunk (should be 4096).
	static const int CHUNK_SIZE;
public:
	SETFile(const std::string &filename, const Palette &palette);
	~SETFile();

	// All individual chunks of a SET are 64x64.
	static const int CHUNK_WIDTH;
	static const int CHUNK_HEIGHT;

	// Gets the number of chunks in the SET.
	int getCount() const;

	// Gets the pixel data for a 64x64 chunk of a SET file.
	uint32_t *getPixels(int index) const;
};

#endif
