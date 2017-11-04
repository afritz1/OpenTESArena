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
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// The first word is the uncompressed length. Some .RMD files (#001-#004) have 0 
	// for this value.
	const uint16_t uncompLen = Bytes::getLE16(srcData.data());

	if (uncompLen == 0)
	{
		// Special case .RMD file? What should the length be?
		DebugNotImplemented();
	}

	// The subsequent words in the file are RLE-compressed. The decomp vector
	// has its size doubled so it can fit the correct number of words.
	std::vector<uint8_t> decomp(uncompLen * 2);
	Compression::decodeRLEWords(srcData.data() + 2, uncompLen, decomp);

	this->flor = std::vector<uint8_t>(decomp.begin(), decomp.begin() + (decomp.size() / 3));
	this->map1 = std::vector<uint8_t>(decomp.begin() + (decomp.size() / 3),
		decomp.begin() + ((2 * decomp.size()) / 3));
	this->map2 = std::vector<uint8_t>(decomp.begin() + ((2 * decomp.size()) / 3), decomp.end());
}

RMDFile::~RMDFile()
{

}

const std::vector<uint8_t> &RMDFile::getFLOR() const
{
	return this->flor;
}

const std::vector<uint8_t> &RMDFile::getMAP1() const
{
	return this->map1;
}

const std::vector<uint8_t> &RMDFile::getMAP2() const
{
	return this->map2;
}
