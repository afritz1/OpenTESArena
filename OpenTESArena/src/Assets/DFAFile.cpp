#include <algorithm>

#include "Compression.h"
#include "DFAFile.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

DFAFile::DFAFile(const std::string &filename, const Palette &palette)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
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
	Compression::decodeRLE(srcData.data() + 12, width * height, frames.at(0));

	// Make copies of the original frame for each update chunk.
	for (int i = 1; i < imageCount; ++i)
	{
		frames.push_back(frames.at(0));
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

			for (uint32_t i = 0; i < updateCount; ++i)
			{
				frame.at(updateOffset + i) = *(srcData.begin() + offset);
				offset++;
			}
		}
	}

	this->width = width;
	this->height = height;

	// Finally, create 32-bit images using each frame's palette indices.
	for (const auto &frame : frames)
	{
		this->pixels.push_back(std::unique_ptr<uint32_t[]>(
			new uint32_t[this->width * this->height]));

		uint32_t *dstPixels = this->pixels.at(this->pixels.size() - 1).get();

		std::transform(frame.begin(), frame.begin() + frame.size(), dstPixels,
			[&palette](uint8_t col) -> uint32_t
		{
			return palette.get()[col].toARGB();
		});
	}
}

DFAFile::~DFAFile()
{

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

uint32_t *DFAFile::getPixels(int index) const
{
	return this->pixels.at(index).get();
}
