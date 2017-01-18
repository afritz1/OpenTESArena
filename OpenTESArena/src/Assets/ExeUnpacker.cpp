#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "ExeUnpacker.h"

#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

enum class DecompressionMode
{
	Default,
	Decrypt,
	Copy
};

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

	// Current position for inserting decompressed data.
	size_t decompIndex = 0;

	// The decompression mode determines how bits are interpreted.
	auto mode = DecompressionMode::Default;
	
	const size_t compressedByteCount = compressedEnd - compressedStart;
	for (size_t i = 0; i < (compressedByteCount - 1); i += 2)
	{
		const uint16_t bitArray = Bytes::getLE16(compressedStart + i);

		// Loop over each bit in the bit array.
		for (int bitIndex = 0; bitIndex < 16; ++bitIndex)
		{
			const bool bit = (bitArray & (1 << bitIndex)) != 0;

			if (mode == DecompressionMode::Default)
			{
				// The value of the bit determines the next mode.
				// - I'm thinking that a decompression "mode" is unnecessary, and the
				//   code can simply branch on the bit itself for copying or decrypting.
				//   Additionally, the mode check above is always true.
				mode = bit ? DecompressionMode::Copy : DecompressionMode::Decrypt;
			}

			if (mode == DecompressionMode::Decrypt)
			{
				// Read the next byte from the compressed data (after the bit array).
				const uint8_t encryptedByte = compressedStart[i + 2];
				i++;

				// Retrieve the XOR key based on the current bit index.
				const std::array<uint8_t, 16> keys =
				{
					15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 16
				};

				const uint8_t key = keys.at(bitIndex);
				const uint8_t decryptedByte = encryptedByte ^ key;

				// Insert the decrypted byte into the decompressed data.
				decomp.at(decompIndex) = decryptedByte;
				decompIndex++;
			}
			else if (mode == DecompressionMode::Copy)
			{
				
			}

			// Change back to default mode.
			mode = DecompressionMode::Default;
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
