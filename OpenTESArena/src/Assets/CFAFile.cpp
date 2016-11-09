#include <algorithm>
#include <array>
#include <cstring>

#include "CFAFile.h"

#include "Compression.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

CFAFile::CFAFile(const std::string &filename, const Palette &palette)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "CFAFile", "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Read CFA header. Fortunately, all CFAs have headers, unlike IMGs and CIFs.
	const uint16_t widthUncompressed = Bytes::getLE16(srcData.data());
	const uint16_t height = Bytes::getLE16(srcData.data() + 2);
	const uint16_t widthCompressed = Bytes::getLE16(srcData.data() + 4);
	const uint16_t unknown1 = Bytes::getLE16(srcData.data() + 6);
	const uint16_t unknown2 = Bytes::getLE16(srcData.data() + 8);
	const uint8_t bitsPerPixel = *(srcData.data() + 10); // Determines demuxing routine.
	const uint8_t frameCount = *(srcData.data() + 11);
	const uint16_t headerSize = Bytes::getLE16(srcData.data() + 12);

	// Adapted from WinArena.

	// Pointer to the look-up conversion table. This is how the packed colors
	// are converted into useful palette indices.
	const uint8_t *lookUpTable = srcData.data() + 76;

	// Line buffer (generously over-allocated for demuxing).
	std::vector<uint8_t> encoded(widthUncompressed + 16);
	std::memset(encoded.data(), 0, encoded.size());

	// Index values from demuxing are stored here each pass, and are
	// eventually translated into color indices.
	std::array<uint8_t, 8> translate;

	// Worse-case buffer for decompressed data (due to possible padding
	// with demux alignment).
	std::vector<uint8_t> decomp(widthCompressed * height * frameCount *
		sizeof(uint32_t) + (widthUncompressed * 16));

	// Decompress the RLE data of the CFA images (they're all packed together).
	Compression::decodeRLE(srcData.data() + headerSize, 
		widthCompressed * height * frameCount, decomp);

	// Temporary buffers for frame palette indices.
	std::vector<std::vector<uint8_t>> frames;

	// Byte offset into bit-packed data. All frames are packed together,
	// so this value can simply be incremented by the compressed width.
	uint32_t offset = 0;

	for (uint32_t frameNum = 0; frameNum < frameCount; ++frameNum)
	{
		// Allocate a new output frame.
		frames.push_back(std::vector<uint8_t>(widthUncompressed * height));

		// Destination buffer for the frame's decompressed palette indices.
		std::vector<uint8_t> &dst = frames.at(frames.size() - 1);
		uint32_t dstOffset = 0;

		for (uint32_t y = 0; y < height; ++y)
		{
			uint32_t count = widthUncompressed;

			// Copy the current line to the scratch buffer.
			std::memcpy(encoded.data(), decomp.data() + offset, widthCompressed);

			// Lambda for which demux routine to do, based on bits per pixel.
			auto runDemux = [&dst, dstOffset, &count, &encoded, &translate, lookUpTable](
				uint32_t end, void(*demux)(const uint8_t*, uint8_t*),
				uint32_t demuxMultiplier, uint32_t upToMin)
			{
				for (uint32_t x = 0; x < end; ++x)
				{
					demux(encoded.data() + (x * demuxMultiplier), translate.data());

					uint32_t upTo = std::min(upToMin, count);
					count -= upTo;

					for (uint32_t i = 0; i < upTo; ++i)
					{
						dst.at((x * upToMin) + i + dstOffset) = lookUpTable[translate.at(i)];
					}
				}
			};

			// Choose the demuxing routine.
			if (bitsPerPixel == 8)
			{
				// No demuxing needed.
				for (uint32_t x = 0; x < widthCompressed; ++x)
				{
					dst.at(x + dstOffset) = encoded.at(x);
				}
			}
			else if (bitsPerPixel == 7)
			{
				runDemux((widthCompressed + 6) / 7, CFAFile::demux7, 7, 8);
			}
			else if (bitsPerPixel == 6)
			{
				runDemux((widthCompressed + 2) / 3, CFAFile::demux6, 3, 4);
			}
			else if (bitsPerPixel == 5)
			{
				runDemux((widthCompressed + 4) / 5, CFAFile::demux5, 5, 8);
			}
			else if (bitsPerPixel == 4)
			{
				runDemux((widthCompressed + 1) / 2, CFAFile::demux4, 2, 4);
			}
			else if (bitsPerPixel == 3)
			{
				runDemux((widthCompressed + 2) / 3, CFAFile::demux3, 3, 8);
			}
			else if (bitsPerPixel == 2)
			{
				runDemux(widthCompressed, CFAFile::demux2, 1, 4);
			}
			else if (bitsPerPixel == 1)
			{
				runDemux(widthCompressed, CFAFile::demux1, 1, 8);
			}

			// Move offsets to the next compressed line of data.
			offset += widthCompressed;
			dstOffset += widthUncompressed;
		}
	}

	this->width = widthUncompressed;
	this->height = height;

	// Finally, create 32-bit images using each frame's palette indices.
	for (const auto &frame : frames)
	{
		this->pixels.push_back(std::unique_ptr<uint32_t>(
			new uint32_t[this->width * this->height]));

		uint32_t *pixels = this->pixels.at(this->pixels.size() - 1).get();

		std::transform(frame.begin(), frame.begin() + frame.size(), pixels,
			[&palette](uint8_t col) -> uint32_t
		{
			return palette[col].toARGB();
		});
	}
}

CFAFile::~CFAFile()
{

}

int CFAFile::getImageCount() const
{
	return static_cast<int>(this->pixels.size());
}

int CFAFile::getWidth() const
{
	return this->width;
}

int CFAFile::getHeight() const
{
	return this->height;
}

uint32_t *CFAFile::getPixels(int index) const
{
	return this->pixels.at(index).get();
}

void CFAFile::demux1(const uint8_t *src, uint8_t *dst)
{
	dst[0] = (src[0] & 0x80) >> 7;
	dst[1] = (src[0] & 0x40) >> 6;
	dst[2] = (src[0] & 0x20) >> 5;
	dst[3] = (src[0] & 0x10) >> 4;
	dst[4] = (src[0] & 0x08) >> 3;
	dst[5] = (src[0] & 0x04) >> 2;
	dst[6] = (src[0] & 0x02) >> 1;
	dst[7] = src[0] & 0x01;
}

void CFAFile::demux2(const uint8_t *src, uint8_t *dst)
{
	dst[0] = (src[0] & 0xC0) >> 6;
	dst[1] = (src[0] & 0x30) >> 4;
	dst[2] = (src[0] & 0x0C) >> 2;
	dst[3] = src[0] & 0x03;
}

void CFAFile::demux3(const uint8_t *src, uint8_t *dst)
{
	dst[0] = (src[0] & 0xE0) >> 5;
	dst[1] = (src[0] & 0x1C) >> 2;
	dst[2] = ((src[0] & 0x03) << 1) | ((src[1] & 0x80) >> 7);
	dst[3] = (src[1] & 0x70) >> 4;
	dst[4] = (src[1] & 0x0E) >> 1;
	dst[5] = ((src[1] & 0x01) << 2) | ((src[2] & 0xC0) >> 6);
	dst[6] = (src[2] & 0x38) >> 3;
	dst[7] = src[2] & 0x07;
}

void CFAFile::demux4(const uint8_t *src, uint8_t *dst)
{
	dst[0] = (src[0] & 0xF0) >> 4;
	dst[1] = src[0] & 0x0F;
	dst[2] = (src[1] & 0xF0) >> 4;
	dst[3] = src[1] & 0x0F;
}

void CFAFile::demux5(const uint8_t *src, uint8_t *dst)
{
	dst[0] = (src[0] & 0xF8) >> 3;
	dst[1] = ((src[0] & 0x07) << 2) | ((src[1] & 0xC0) >> 6);
	dst[2] = (src[1] & 0x3E) >> 1;
	dst[3] = ((src[1] & 0x01) << 4) | ((src[2] & 0xF0) >> 4);
	dst[4] = ((src[2] & 0x0F) << 1) | ((src[3] & 0x80) >> 7);
	dst[5] = (src[3] & 0x7C) >> 2;
	dst[6] = ((src[3] & 0x03) << 3) | ((src[4] & 0xE0) >> 5);
	dst[7] = src[4] & 0x1F;
}

void CFAFile::demux6(const uint8_t *src, uint8_t *dst)
{
	dst[0] = (src[0] & 0xFC) >> 2;
	dst[1] = ((src[0] & 0x03) << 4) | ((src[1] & 0xF0) >> 4);
	dst[2] = ((src[1] & 0x0F) << 2) | ((src[2] & 0xC0) >> 6);
	dst[3] = src[2] & 0x3F;
}

void CFAFile::demux7(const uint8_t *src, uint8_t *dst)
{
	dst[0] = (src[0] & 0xFE) >> 1;
	dst[1] = ((src[0] & 0x01) << 6) | ((src[1] & 0xFC) >> 2);
	dst[2] = ((src[1] & 0x03) << 5) | ((src[2] & 0xF8) >> 3);
	dst[3] = ((src[2] & 0x07) << 4) | ((src[3] & 0xF0) >> 4);
	dst[4] = ((src[3] & 0x0F) << 3) | ((src[4] & 0xE0) >> 5);
	dst[5] = ((src[4] & 0x1F) << 2) | ((src[5] & 0xC0) >> 6);
	dst[6] = ((src[5] & 0x3F) << 1) | ((src[6] & 0x80) >> 7);
	dst[7] = src[6] & 0x7F;
}
