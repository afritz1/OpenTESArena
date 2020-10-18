#include <algorithm>
#include <string>

#include "Compression.h"
#include "RMDFile.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/vfs/manager.hpp"

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

	this->flor.init(RMDFile::WIDTH, RMDFile::DEPTH);
	this->map1.init(RMDFile::WIDTH, RMDFile::DEPTH);
	this->map2.init(RMDFile::WIDTH, RMDFile::DEPTH);

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

		std::copy(florStart, florEnd, reinterpret_cast<uint8_t*>(this->flor.get()));
		std::copy(florEnd, map1End, reinterpret_cast<uint8_t*>(this->map1.get()));
		std::copy(map1End, map2End, reinterpret_cast<uint8_t*>(this->map2.get()));
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

		std::copy(florStart, florEnd, reinterpret_cast<uint8_t*>(this->flor.get()));
		std::copy(florEnd, map1End, reinterpret_cast<uint8_t*>(this->map1.get()));
		std::copy(map1End, map2End, reinterpret_cast<uint8_t*>(this->map2.get()));
	}

	return true;
}

BufferView2D<const ArenaTypes::VoxelID> RMDFile::getFLOR() const
{
	return BufferView2D<const ArenaTypes::VoxelID>(
		this->flor.get(), this->flor.getWidth(), this->flor.getHeight());
}

BufferView2D<const ArenaTypes::VoxelID> RMDFile::getMAP1() const
{
	return BufferView2D<const ArenaTypes::VoxelID>(
		this->map1.get(), this->map1.getWidth(), this->map1.getHeight());
}

BufferView2D<const ArenaTypes::VoxelID> RMDFile::getMAP2() const
{
	return BufferView2D<const ArenaTypes::VoxelID>(
		this->map2.get(), this->map2.getWidth(), this->map2.getHeight());
}
