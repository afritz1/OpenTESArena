#include <algorithm>

#include "SETFile.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

const int SETFile::CHUNK_WIDTH = 64;
const int SETFile::CHUNK_HEIGHT = 64;
const int SETFile::CHUNK_SIZE = SETFile::CHUNK_WIDTH * SETFile::CHUNK_HEIGHT;

SETFile::SETFile(const std::string &filename, const Palette &palette)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

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

	// Number of uncompressed chunks packed in the SET.
	const int chunkCount = static_cast<int>(srcData.size()) / SETFile::CHUNK_SIZE;

	// Create an image for each uncompressed chunk using the given palette.
	for (int chunkIndex = 0; chunkIndex < chunkCount; chunkIndex++)
	{
		this->chunks.push_back(std::make_unique<uint32_t[]>(SETFile::CHUNK_SIZE));

		const int byteOffset = SETFile::CHUNK_SIZE * chunkIndex;
		const auto chunkStart = srcData.begin() + byteOffset;
		const auto chunkEnd = chunkStart + SETFile::CHUNK_SIZE;

		std::transform(chunkStart, chunkEnd, this->chunks.at(chunkIndex).get(),
			[&palette](uint8_t col) -> uint32_t
		{
			return palette.get()[col].toARGB();
		});
	}
}

SETFile::~SETFile()
{

}

int SETFile::getImageCount() const
{
	return static_cast<int>(this->chunks.size());
}

uint32_t *SETFile::getPixels(int index) const
{
	return this->chunks.at(index).get();
}
