#include "MIFFile.h"

#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

MIFFile::MIFFile(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Start of the header data (at "MHDR").
	const uint8_t *headerStart = srcData.data();

	// Get the map dimensions from the header. Constant for all levels.
	const uint16_t mapWidth = Bytes::getLE16(headerStart + 27);
	const uint16_t mapDepth = Bytes::getLE16(headerStart + 29);

	// Start of the level data (at each "LEVL"). Some MIF files have multiple levels,
	// so this needs to be in a loop.
	/*int offset = 67;
	while ((srcData.begin() + offset) < srcData.end())
	{
		const uint8_t *levelStart = headerStart + offset;

		// Get the level size after "LEVL". This seems to be related to the size of 
		// compressed level data, but it doesn't tell when the level data stops.
		MIFFile::Level level;
		level.size = Bytes::getLE16(levelStart + 4);

		// -- to do --
		// The MIF file has some other tokens besides MHDR and LEVL, not always present.
		// Check for these conditionally. Each might be null-terminated, not sure.

		// To do: Don't use level.size here. Try the end of the last chunk (flor, map1, loot, etc.).
		offset += level.size;
	}*/

	this->width = mapWidth;
	this->depth = mapDepth;

	DebugNotImplemented();
}

MIFFile::~MIFFile()
{

}

int MIFFile::getWidth() const
{
	return this->width;
}

int MIFFile::getDepth() const
{
	return this->depth;
}
