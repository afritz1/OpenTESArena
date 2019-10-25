#include <algorithm>
#include <string>

#include "RCIFile.h"

#include "components/debug/Debug.h"
#include "components/vfs/manager.hpp"

const int RCIFile::WIDTH = 320;
const int RCIFile::HEIGHT = 100;
const int RCIFile::FRAME_SIZE = RCIFile::WIDTH * RCIFile::HEIGHT;

bool RCIFile::init(const char *filename)
{
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.get());

	// Number of uncompressed frames packed in the .RCI.
	const int frameCount = src.getCount() / RCIFile::FRAME_SIZE;

	// Create an image for each uncompressed frame using the given palette.
	for (int i = 0; i < frameCount; i++)
	{
		this->pixels.push_back(std::make_unique<uint8_t[]>(RCIFile::FRAME_SIZE));

		const uint8_t *srcPixels = srcPtr + (RCIFile::FRAME_SIZE * i);
		uint8_t *dstPixels = this->pixels.back().get();
		std::copy(srcPixels, srcPixels + RCIFile::FRAME_SIZE, dstPixels);
	}

	return true;
}

int RCIFile::getImageCount() const
{
	return static_cast<int>(this->pixels.size());
}

const uint8_t *RCIFile::getPixels(int index) const
{
	DebugAssertIndex(this->pixels, index);
	return this->pixels[index].get();
}
