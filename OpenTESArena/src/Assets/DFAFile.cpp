#include <algorithm>

#include "Compression.h"
#include "DFAFile.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

DFAFile::DFAFile(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssertMsg(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Read DFA header data.
	const uint16_t imageCount = Bytes::getLE16(srcData.data());
	//const uint16_t unknown1 = Bytes::getLE16(srcData.data() + 2); // Uncomment these when in use.
	//const uint16_t unknown2 = Bytes::getLE16(srcData.data() + 4);
	const uint16_t width = Bytes::getLE16(srcData.data() + 6);
	const uint16_t height = Bytes::getLE16(srcData.data() + 8);
	const uint16_t compressedLength = Bytes::getLE16(srcData.data() + 10); // First frame.

	// Frame data with palette indices.
	std::vector<std::vector<uint8_t>> frames;

	// Uncompress the initial frame.
	frames.push_back(std::vector<uint8_t>(width * height));
	Compression::decodeRLE(srcData.data() + 12, width * height, frames.front());

	// Make copies of the original frame for each update chunk.
	for (int i = 1; i < imageCount; i++)
	{
		frames.push_back(frames.front());
	}

	// Offset to the beginning of the chunk data; advances as the chunk data is read.
	uint32_t offset = 12 + compressedLength;

	// Start reading chunks for each update group. Skip the first frame because 
	// that's the full image.
	for (uint32_t frameIndex = 1; frameIndex < imageCount; ++frameIndex)
	{
		// Select the frame buffer at the current frame index.
		std::vector<uint8_t> &frame = frames.at(frameIndex);

		// Pointer to the beginning of the chunk data. Each update chunk
		// changes a group of pixels in a copy of the original image.
		const uint8_t *chunkData = srcData.data() + offset;
		const uint16_t chunkSize = Bytes::getLE16(chunkData);
		const uint16_t chunkCount = Bytes::getLE16(chunkData + 2);

		// Move the offset past the chunk header.
		offset += 4;

		for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
		{
			const uint8_t *updateData = srcData.data() + offset;
			const uint16_t updateOffset = Bytes::getLE16(updateData);
			const uint16_t updateCount = Bytes::getLE16(updateData + 2);

			// Move the offset past the update header.
			offset += 4;

			for (uint32_t i = 0; i < updateCount; i++)
			{
				frame.at(updateOffset + i) = *(srcData.begin() + offset);
				offset++;
			}
		}
	}

	this->width = width;
	this->height = height;

	// Store each 8-bit image.
	for (const auto &frame : frames)
	{
		this->pixels.push_back(std::make_unique<uint8_t[]>(this->width * this->height));
		uint8_t *dstPixels = this->pixels.back().get();
		std::copy(frame.begin(), frame.end(), dstPixels);
	}
}

int DFAFile::getImageCount() const
{
	return static_cast<int>(this->pixels.size());
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
	return this->pixels.at(index).get();
}
