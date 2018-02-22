#include <algorithm>
#include <vector>

#include "Compression.h"
#include "RMDFile.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

const int RMDFile::WIDTH = 64;
const int RMDFile::DEPTH = RMDFile::WIDTH;

RMDFile::RMDFile(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// The first word is the uncompressed length. Some .RMD files (#001 - #004) have 0 for 
	// this value. They are used for storing uncompressed quarters of cities when in the 
	// wilderness.
	const uint16_t uncompLen = Bytes::getLE16(srcData.data());

	// If the length is 0, the file is uncompressed and its size must be 24576 
	// (64 width * 64 depth * 2 bytes/word * 3 floors). Otherwise, it is compressed.
	if (uncompLen == 0)
	{
		const uint16_t requiredSize = 24576;
		DebugAssert(srcData.size() == requiredSize, "Invalid .RMD file.");

		// Write the uncompressed data into each floor.
		const auto florStart = srcData.begin();
		const auto florEnd = srcData.begin() + (srcData.size() / 3);
		const auto map1End = srcData.begin() + ((2 * srcData.size()) / 3);
		const auto map2End = srcData.end();

		std::copy(florStart, florEnd, reinterpret_cast<uint8_t*>(this->flor.data()));
		std::copy(florEnd, map1End, reinterpret_cast<uint8_t*>(this->map1.data()));
		std::copy(map1End, map2End, reinterpret_cast<uint8_t*>(this->map2.data()));
	}
	else
	{
		// The subsequent words in the file are RLE-compressed. The decompressed vector's
		// size is doubled so it can fit the correct number of words.
		std::vector<uint8_t> decomp(uncompLen * 2);
		Compression::decodeRLEWords(srcData.data() + 2, uncompLen, decomp);

		// Write the decompressed data into each floor.
		const auto florStart = decomp.begin();
		const auto florEnd = decomp.begin() + (decomp.size() / 3);
		const auto map1End = decomp.begin() + ((2 * decomp.size()) / 3);
		const auto map2End = decomp.end();

		std::copy(florStart, florEnd, reinterpret_cast<uint8_t*>(this->flor.data()));
		std::copy(florEnd, map1End, reinterpret_cast<uint8_t*>(this->map1.data()));
		std::copy(map1End, map2End, reinterpret_cast<uint8_t*>(this->map2.data()));
	}
}

RMDFile::~RMDFile()
{

}

const RMDFile::ArrayType &RMDFile::getFLOR() const
{
	return this->flor;
}

const RMDFile::ArrayType &RMDFile::getMAP1() const
{
	return this->map1;
}

const RMDFile::ArrayType &RMDFile::getMAP2() const
{
	return this->map2;
}
