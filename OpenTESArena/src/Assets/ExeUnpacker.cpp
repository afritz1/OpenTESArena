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
	
	// A 16-bit array of compressed data.
	uint16_t bitArray = Bytes::getLE16(compressedStart);

	// Decompression mode for determining how bits are interpreted.
	auto mode = DecompressionMode::Default;

	// Continually read bit arrays from the compressed data. Break once a
	// compressed byte equals 0xFF in copy mode.
	int byteIndex = 0; // Offset from start of compressed data.
	int bitIndex = 0; // Position in 16-bit array.
	while (true)
	{
		// Advance the bit array if done with the current one.
		if (bitIndex > 15)
		{
			byteIndex += 2;
			bitIndex = 0;
			bitArray = Bytes::getLE16(compressedStart + byteIndex);
		}

		const bool bit = (bitArray & (1 << bitIndex)) != 0;

		// Choose which decompression mode to use for the current bit.
		if (mode == DecompressionMode::Default)
		{
			mode = bit ? DecompressionMode::Copy : DecompressionMode::Decrypt;
		}

		if (mode == DecompressionMode::Decrypt)
		{
			// Read the next byte from the compressed data (after the bit array).
			const uint8_t encryptedByte = compressedStart[byteIndex + 2];
			byteIndex++;

			// Lambda for decrypting an encrypted byte with an XOR operation based on 
			// the current bit index.
			auto decrypt = [](uint8_t encryptedByte, int bitIndex)
			{
				const uint8_t key = (bitIndex < 15) ? (15 - bitIndex) : 16;
				const uint8_t decryptedByte = encryptedByte ^ key;
				return decryptedByte;
			};

			// Decrypt the byte.
			const uint8_t decryptedByte = decrypt(encryptedByte, bitIndex);

			// Append the decrypted byte onto the decompressed data.
			decomp.at(decompIndex) = decryptedByte;
			decompIndex++;

			// Advance to the next bit in the bit array.
			bitIndex++;
		}
		else
		{
			// Copy mode.
			Debug::crash("Exe Unpacker", "Copy mode not implemented.");
		}

		mode = DecompressionMode::Default;
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
