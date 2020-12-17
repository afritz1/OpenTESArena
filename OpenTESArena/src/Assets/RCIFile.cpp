#include <algorithm>
#include <string>

#include "RCIFile.h"

#include "components/debug/Debug.h"
#include "components/vfs/manager.hpp"

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
	this->images.init(frameCount);
	for (int i = 0; i < this->images.getCount(); i++)
	{
		Buffer2D<uint8_t> &image = this->images.get(i);
		image.init(RCIFile::WIDTH, RCIFile::HEIGHT);

		const uint8_t *srcPixels = srcPtr + (RCIFile::FRAME_SIZE * i);
		std::copy(srcPixels, srcPixels + RCIFile::FRAME_SIZE, image.get());
	}

	return true;
}

int RCIFile::getImageCount() const
{
	return this->images.getCount();
}

const uint8_t *RCIFile::getPixels(int index) const
{
	DebugAssert(index >= 0);
	DebugAssert(index < this->images.getCount());
	return this->images.get(index).get();
}
