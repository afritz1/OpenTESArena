#include <algorithm>
#include <array>
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

	// Current position for inserting decompressed data.
	size_t decompIndex = 0;
	
	// A 16-bit array of compressed data.
	uint16_t bitArray = Bytes::getLE16(compressedStart);

	// Offset from start of compressed data (start at 2 because of the bit array).
	int byteIndex = 2;
	
	// Number of bits consumed in the current 16-bit array.
	int bitsRead = 0;

	// Continually read bit arrays from the compressed data and interpret each bit. 
	// Break once a compressed byte equals 0xFF in copy mode.
	while (true)
	{
		// Lambda for getting the next byte from compressed data.
		auto getNextByte = [compressedStart, &byteIndex]()
		{
			const uint8_t byte = compressedStart[byteIndex];
			byteIndex++;

			return byte;
		};

		// Lambda for getting the next bit in the theoretical bit stream.
		auto getNextBit = [compressedStart, &bitArray, &bitsRead, &getNextByte]()
		{
			const bool bit = (bitArray & (1 << bitsRead)) != 0;
			bitsRead++;

			// Advance the bit array if done with the current one.
			if (bitsRead == 16)
			{
				bitsRead = 0;

				// Get two bytes in little endian format.
				const uint8_t byte1 = getNextByte();
				const uint8_t byte2 = getNextByte();
				bitArray = byte1 | (byte2 << 8);
			}

			return bit;
		};
		
		// Decide which mode to use for the current bit.
		if (getNextBit())
		{
			// "Copy" mode.
			// Calculate which bytes in the decompressed data to duplicate and append.
			Debug::crash("Exe Unpacker", "Copy mode not implemented.");
		}
		else
		{
			// "Decryption" mode.
			// Read the next byte from the compressed data.
			const uint8_t encryptedByte = getNextByte();

			// Lambda for decrypting an encrypted byte with an XOR operation based on 
			// the current bit index. "bitsRead" is between 0 and 15. It is 0 if the
			// 16th bit of the previous array was used to get here.
			auto decrypt = [](uint8_t encryptedByte, int bitsRead)
			{
				const uint8_t key = 16 - bitsRead;
				const uint8_t decryptedByte = encryptedByte ^ key;
				return decryptedByte;
			};

			// Decrypt the byte.
			const uint8_t decryptedByte = decrypt(encryptedByte, bitsRead);

			// Append the decrypted byte onto the decompressed data.
			decomp.at(decompIndex) = decryptedByte;
			decompIndex++;
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
