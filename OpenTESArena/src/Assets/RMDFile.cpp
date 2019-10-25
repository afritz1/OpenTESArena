#include <algorithm>
#include <string>

#include "Compression.h"
#include "RMDFile.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/vfs/manager.hpp"

const int RMDFile::BYTES_PER_FLOOR = 8192;
const int RMDFile::WIDTH = 64;
const int RMDFile::DEPTH = RMDFile::WIDTH;
const int RMDFile::ELEMENTS_PER_FLOOR = RMDFile::BYTES_PER_FLOOR / 2;

bool RMDFile::init(const char *filename)
{
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.get());
	const uint8_t *srcEnd = reinterpret_cast<const uint8_t*>(src.end());

	this->flor = std::vector<uint16_t>(RMDFile::ELEMENTS_PER_FLOOR);
	this->map1 = std::vector<uint16_t>(RMDFile::ELEMENTS_PER_FLOOR);
	this->map2 = std::vector<uint16_t>(RMDFile::ELEMENTS_PER_FLOOR);

	// The first word is the uncompressed length. Some .RMD files (#001 - #004) have 0 for 
	// this value. They are used for storing uncompressed quarters of cities when in the 
	// wilderness.
	const uint16_t uncompLen = Bytes::getLE16(srcPtr);

	// If the length is 0, the file is uncompressed and its size must be 24576 
	// (64 width * 64 depth * 2 bytes/word * 3 floors). Otherwise, it is compressed.
	if (uncompLen == 0)
	{
		const uint16_t requiredSize = RMDFile::BYTES_PER_FLOOR * 3;
		if (src.getCount() != requiredSize)
		{
			DebugLogError("Invalid .RMD file (size: " + std::to_string(src.getCount()) + ").");
			return false;
		}

		// Write the uncompressed data into each floor.
		const auto florStart = srcPtr;
		const auto florEnd = srcPtr + (src.getCount() / 3);
		const auto map1End = srcPtr + ((2 * src.getCount()) / 3);
		const auto map2End = srcEnd;

		std::copy(florStart, florEnd, reinterpret_cast<uint8_t*>(this->flor.data()));
		std::copy(florEnd, map1End, reinterpret_cast<uint8_t*>(this->map1.data()));
		std::copy(map1End, map2End, reinterpret_cast<uint8_t*>(this->map2.data()));
	}
	else
	{
		// The subsequent words in the file are RLE-compressed. The decompressed vector's
		// size is doubled so it can fit the correct number of words.
		std::vector<uint8_t> decomp(uncompLen * 2);
		Compression::decodeRLEWords(srcPtr + 2, uncompLen, decomp);

		// Write the decompressed data into each floor.
		const auto florStart = decomp.begin();
		const auto florEnd = decomp.begin() + (decomp.size() / 3);
		const auto map1End = decomp.begin() + ((2 * decomp.size()) / 3);
		const auto map2End = decomp.end();

		std::copy(florStart, florEnd, reinterpret_cast<uint8_t*>(this->flor.data()));
		std::copy(florEnd, map1End, reinterpret_cast<uint8_t*>(this->map1.data()));
		std::copy(map1End, map2End, reinterpret_cast<uint8_t*>(this->map2.data()));
	}

	return true;
}

const std::vector<uint16_t> &RMDFile::getFLOR() const
{
	return this->flor;
}

const std::vector<uint16_t> &RMDFile::getMAP1() const
{
	return this->map1;
}

const std::vector<uint16_t> &RMDFile::getMAP2() const
{
	return this->map2;
}
