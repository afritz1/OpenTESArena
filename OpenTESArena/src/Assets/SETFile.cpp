#include <algorithm>

#include "SETFile.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

const int SETFile::CHUNK_WIDTH = 64;
const int SETFile::CHUNK_HEIGHT = 64;
const int SETFile::CHUNK_SIZE = SETFile::CHUNK_WIDTH * SETFile::CHUNK_HEIGHT;

SETFile::SETFile(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssertMsg(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// There is one .SET file with a file size of 0x3FFF, so it is a special case.
	const bool isSpecialCase = filename == "TBS2.SET";

	if (isSpecialCase)
	{
		// Add a dummy byte onto the end.
		srcData.push_back(0);
	}

	// Number of uncompressed chunks packed in the .SET.
	const int chunkCount = static_cast<int>(srcData.size()) / SETFile::CHUNK_SIZE;

	// Create an image for each uncompressed chunk.
	for (int i = 0; i < chunkCount; i++)
	{
		this->pixels.push_back(std::make_unique<uint8_t[]>(SETFile::CHUNK_SIZE));

		const uint8_t *srcPixels = srcData.data() + (SETFile::CHUNK_SIZE * i);
		uint8_t *dstPixels = this->pixels.back().get();
		std::copy(srcPixels, srcPixels + SETFile::CHUNK_SIZE, dstPixels);
	}
}

int SETFile::getImageCount() const
{
	return static_cast<int>(this->pixels.size());
}

const uint8_t *SETFile::getPixels(int index) const
{
	return this->pixels.at(index).get();
}
