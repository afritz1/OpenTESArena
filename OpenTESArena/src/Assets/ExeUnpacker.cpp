#include <algorithm>
#include <cstdint>
#include <vector>

#include "ExeUnpacker.h"

#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

ExeUnpacker::ExeUnpacker(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "Exe Unpacker", "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Beginning and end of compressed data in the executable.
	const uint8_t *compressedStart = srcData.data() + 752;
	const uint8_t *compressedEnd = srcData.data() + (srcData.size() - 8);

	// Last word of compressed data must be 0xFFFF.
	const uint16_t lastCompWord = Bytes::getLE16(compressedEnd - 2);
	Debug::check(lastCompWord == 0xFFFF, "Exe Unpacker", 
		"Invalid last compressed word \"" + String::toHexString(lastCompWord) + "\".");

	// Calculate length of decompressed data -- more precise method (for A.EXE).
	const size_t decompLen = [compressedEnd]()
	{
		const uint16_t segment = Bytes::getLE16(compressedEnd);
		const uint16_t offset = Bytes::getLE16(compressedEnd + 2);
		return (segment * 16) + offset;
	}();

	// Buffer for the decompressed data (also little endian).
	std::vector<uint8_t> decomp(decompLen);
	
	const size_t compressedByteCount = compressedEnd - compressedStart;
	for (size_t i = 0; i < compressedByteCount; i += 2)
	{
		const uint16_t bitArray = Bytes::getLE16(compressedStart + i);

		// Loop over each bit in the bit array.
		for (int bitIndex = 0; bitIndex < 16; ++bitIndex)
		{
			const bool bit = (bitArray & (1 << bitIndex)) != 0;

			// Default mode...

			// Decrypt mode...

			// Copy mode...

			// Maybe make lambdas for each mode, perhaps? Or private class methods?
			// A "DecompressionMode" enum class?
		}
	}

	// Keep this until the decompressor works.
	Debug::crash("Exe Unpacker", "Not implemented.");
}

ExeUnpacker::~ExeUnpacker()
{

}

const std::string &ExeUnpacker::getText() const
{
	return this->text;
}
