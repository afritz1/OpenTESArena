#ifndef EXE_UNPACKER_H
#define EXE_UNPACKER_H

#include <cstdint>

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

// For decompressing DOS executables compressed with PKLITE.
class ExeUnpacker
{
private:
	Buffer<uint8_t> exeData;
public:
	// Reads in a compressed EXE file and decompresses it.
	bool init(const char *filename);

	// Gets the decompressed executable data.
	BufferView<const uint8_t> getData() const;
};

#endif
