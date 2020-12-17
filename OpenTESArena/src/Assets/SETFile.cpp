#include <algorithm>
#include <cstring>
#include <string>

#include "SETFile.h"

#include "components/debug/Debug.h"
#include "components/vfs/manager.hpp"

bool SETFile::init(const char *filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	if (stream == nullptr)
	{
		DebugLogError("Could not open \"" + std::string(filename) + "\".");
		return false;
	}

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// There is one .SET file with a file size of 0x3FFF, so it is a special case.
	const bool isSpecialCase = std::strcmp(filename, "TBS2.SET") == 0;

	if (isSpecialCase)
	{
		// Add a dummy byte onto the end.
		srcData.push_back(0);
	}

	// Number of uncompressed chunks packed in the .SET.
	const int chunkCount = static_cast<int>(srcData.size()) / SETFile::CHUNK_SIZE;
	this->images.init(chunkCount);
	for (int i = 0; i < this->images.getCount(); i++)
	{
		Buffer2D<uint8_t> &image = this->images.get(i);
		image.init(SETFile::CHUNK_WIDTH, SETFile::CHUNK_HEIGHT);

		const uint8_t *srcPixels = srcData.data() + (SETFile::CHUNK_SIZE * i);
		std::copy(srcPixels, srcPixels + SETFile::CHUNK_SIZE, image.get());
	}

	return true;
}

int SETFile::getImageCount() const
{
	return this->images.getCount();
}

const uint8_t *SETFile::getPixels(int index) const
{
	DebugAssert(index >= 0);
	DebugAssert(index < this->images.getCount());
	return this->images.get(index).get();
}
