#ifndef EXE_UNPACKER_H
#define EXE_UNPACKER_H

#include <cstdint>
#include <vector>

// For decompressing DOS executables compressed with PKLITE.

class ExeUnpacker
{
private:
	std::vector<uint8_t> exeData;
public:
	// Reads in a compressed EXE file and decompresses it.
	bool init(const char *filename);

	// Gets the decompressed executable data.
	const std::vector<uint8_t> &getData() const;
};

#endif
