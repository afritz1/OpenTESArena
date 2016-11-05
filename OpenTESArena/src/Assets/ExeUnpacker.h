#ifndef EXE_UNPACKER_H
#define EXE_UNPACKER_H

#include <string>

// For decompressing DOS executables compressed with PKLITE.

class ExeUnpacker
{
private:
	std::string text;
public:
	// Reads in a compressed EXE file and decompresses it into a "text" member.
	ExeUnpacker(const std::string &filename);
	~ExeUnpacker();

	// Gets the decompressed executable file data.
	const std::string &getText() const;
};

#endif
