#include <algorithm>
#include <string>

#include "Compression.h"
#include "DFAFile.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/vfs/manager.hpp"

bool DFAFile::init(const char *filename)
{
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.get());

	// Read DFA header data.
	const uint16_t imageCount = Bytes::getLE16(srcPtr);
	//const uint16_t unknown1 = Bytes::getLE16(srcPtr + 2); // Uncomment these when in use.
	//const uint16_t unknown2 = Bytes::getLE16(srcPtr + 4);
	const uint16_t width = Bytes::getLE16(srcPtr + 6);
	const uint16_t height = Bytes::getLE16(srcPtr + 8);
	const uint16_t compressedLength = Bytes::getLE16(srcPtr + 10); // First frame.

	// Frame data with palette indices.
	this->images.init(imageCount);

	// Uncompress the initial frame.
	Buffer2D<uint8_t> &firstImage = this->images.get(0);
	firstImage.init(width, height);
	Compression::decodeRLE(srcPtr + 12, width * height, firstImage.get(),
		firstImage.getWidth() * firstImage.getHeight());

	// Make copies of the original frame for each update chunk.
	for (int i = 1; i < imageCount; i++)
	{
		Buffer2D<uint8_t> &image = this->images.get(i);
		image.init(firstImage.getWidth(), firstImage.getHeight());
		std::copy(firstImage.get(), firstImage.end(), image.get());
	}

	// Offset to the beginning of the chunk data; advances as the chunk data is read.
	uint32_t offset = 12 + compressedLength;

	// Start reading chunks for each update group. Skip the first frame because 
	// that's the full image.
	for (uint32_t frameIndex = 1; frameIndex < imageCount; frameIndex++)
	{
		// Select the frame buffer at the current frame index.
		Buffer2D<uint8_t> &image = this->images.get(frameIndex);

		// Pointer to the beginning of the chunk data. Each update chunk
		// changes a group of pixels in a copy of the original image.
		const uint8_t *chunkData = srcPtr + offset;
		const uint16_t chunkSize = Bytes::getLE16(chunkData);
		const uint16_t chunkCount = Bytes::getLE16(chunkData + 2);

		// Move the offset past the chunk header.
		offset += 4;

		for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
		{
			const uint8_t *updateData = srcPtr + offset;
			const uint16_t updateOffset = Bytes::getLE16(updateData);
			const uint16_t updateCount = Bytes::getLE16(updateData + 2);

			// Move the offset past the update header.
			offset += 4;

			uint8_t *imagePtr = image.get();
			for (uint32_t i = 0; i < updateCount; i++)
			{
				const int dstIndex = updateOffset + i;
				imagePtr[dstIndex] = *(srcPtr + offset);
				offset++;
			}
		}
	}

	this->width = width;
	this->height = height;
	return true;
}

int DFAFile::getImageCount() const
{
	return this->images.getCount();
}

int DFAFile::getWidth() const
{
	return this->width;
}

int DFAFile::getHeight() const
{
	return this->height;
}

const uint8_t *DFAFile::getPixels(int index) const
{
	DebugAssert(index >= 0);
	DebugAssert(index < this->images.getCount());
	return this->images.get(index).get();
}
