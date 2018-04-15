#include <algorithm>

#include "RCIFile.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

const int RCIFile::WIDTH = 320;
const int RCIFile::HEIGHT = 100;
const int RCIFile::FRAME_SIZE = RCIFile::WIDTH * RCIFile::HEIGHT;

RCIFile::RCIFile(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Number of uncompressed frames packed in the .RCI.
	const int frameCount = static_cast<int>(srcData.size()) / RCIFile::FRAME_SIZE;

	// Create an image for each uncompressed frame using the given palette.
	for (int i = 0; i < frameCount; i++)
	{
		this->pixels.push_back(std::make_unique<uint8_t[]>(RCIFile::FRAME_SIZE));

		const uint8_t *srcPixels = srcData.data() + (RCIFile::FRAME_SIZE * i);
		uint8_t *dstPixels = this->pixels.back().get();
		std::copy(srcPixels, srcPixels + RCIFile::FRAME_SIZE, dstPixels);
	}
}

int RCIFile::getImageCount() const
{
	return static_cast<int>(this->pixels.size());
}

const uint8_t *RCIFile::getPixels(int index) const
{
	return this->pixels.at(index).get();
}
