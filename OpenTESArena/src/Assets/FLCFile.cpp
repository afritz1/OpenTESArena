#include <algorithm>
#include <array>

#include "FLCFile.h"

#include "Compression.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

enum class FileType : uint16_t
{
	FLC_TYPE = 0xAF12
};

enum class ChunkType : uint16_t
{
	COLOR_256 = 0x04, // 256 color palette.
	FLI_SS2 = 0x07, // DELTA_FLC.
	COLOR_64 = 0x0B, // 64 color palette.
	FLI_LC = 0x0C, // DELTA_FLI.
	BLACK = 0x0D, // Entire frame is color 0.
	FLI_BRUN = 0x0F, // BYTE_RUN.
	FLI_COPY = 0x10, // Uncompressed pixels.
	PSTAMP = 0x12 // A 64x32 icon for the first full frame.
};

enum class FrameType : uint16_t
{
	PREFIX_CHUNK = 0xF100,
	FRAME_TYPE = 0xF1FA
};

struct FLICHeader
{
	uint32_t size;          // Size of FLIC including this header.
	uint16_t type;          // File type 0xAF11, 0xAF12, 0xAF30, 0xAF44, ...
	uint16_t frames;        // Number of frames in first segment.
	uint16_t width;         // FLIC width in pixels.
	uint16_t height;        // FLIC height in pixels.
	uint16_t depth;         // Bits per pixel (usually 8).
	uint16_t flags;         // Set to zero or to three.
	uint32_t speed;         // Delay between frames (in milliseconds).
	uint16_t reserved1;     // Set to zero.
	uint32_t created;       // Date of FLIC creation (FLC only).
	uint32_t creator;       // Serial number or compiler id (FLC only).
	uint32_t updated;       // Date of FLIC update (FLC only).
	uint32_t updater;       // Serial number (FLC only), see creator.
	uint16_t aspect_dx;     // Width of square rectangle (FLC only).
	uint16_t aspect_dy;     // Height of square rectangle (FLC only).
	uint16_t ext_flags;     // EGI: flags for specific EGI extensions.
	uint16_t keyframes;     // EGI: key-image frequency.
	uint16_t totalframes;   // EGI: total number of frames (segments).
	uint32_t req_memory;    // EGI: maximum chunk size (uncompressed).
	uint16_t max_regions;   // EGI: max. number of regions in a CHK_REGION chunk.
	uint16_t transp_num;    // EGI: number of transparent levels.
	std::array<uint8_t, 20> reserved2; // Set to zero.
	uint32_t oframe1;       // Offset to frame 1 (FLC only).
	uint32_t oframe2;       // Offset to frame 2 (FLC only).
	std::array<uint8_t, 40> reserved3; // Set to zero.
};

struct FrameHeader
{
	uint32_t size; // Total size of frame.
	FrameType type; // Frame identifier.
	uint16_t chunkCount; // Number of chunks in this frame.
	std::array<uint8_t, 8> reserved; // Set to zero.

	FrameHeader(uint32_t size, uint16_t type, uint16_t chunkCount)
	{
		this->size = size;
		this->type = static_cast<FrameType>(type);
		this->chunkCount = chunkCount;
	}
};

struct ChunkHeader
{
	uint32_t size; // Total size of chunk.
	ChunkType type; // Chunk identifier.

	ChunkHeader(uint32_t chunkSize, uint16_t chunkType)
	{
		this->size = chunkSize;
		this->type = static_cast<ChunkType>(chunkType);
	}
};

FLCFile::FLCFile(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Get the header data. Some of it is just miscellaneous (last updated, etc.),
	// or only used in later versions with the EGI modifications.
	FLICHeader header;
	header.size = Bytes::getLE32(srcData.data());
	header.type = Bytes::getLE16(srcData.data() + 4);
	header.frames = Bytes::getLE16(srcData.data() + 6);
	header.width = Bytes::getLE16(srcData.data() + 8);
	header.height = Bytes::getLE16(srcData.data() + 10);
	header.depth = Bytes::getLE16(srcData.data() + 12);
	header.flags = Bytes::getLE16(srcData.data() + 14);
	header.speed = Bytes::getLE32(srcData.data() + 16);

	// This class will only support the format used by Arena (0xAF12) for now.
	DebugAssert(header.type == static_cast<int>(FileType::FLC_TYPE), 
		"Unsupported file type \"" + std::to_string(header.type) + "\".");

	this->frameDuration = static_cast<double>(header.speed) / 1000.0;
	this->width = header.width;
	this->height = header.height;

	// Palette which is filled by one of the FLIC color chunks.
	Palette palette;

	// Current state of the frame's palette indices. Completely updated by byte runs
	// and partially updated by delta frames.
	std::vector<uint8_t> framePixels(this->width * this->height);

	// Start decoding frames. The data starts after the header.
	uint32_t dataOffset = sizeof(FLICHeader);
	while ((srcData.begin() + dataOffset) < srcData.end())
	{
		const uint8_t *framePtr = srcData.data() + dataOffset;

		const FrameHeader frameHeader(Bytes::getLE32(framePtr),
			Bytes::getLE16(framePtr + 4), Bytes::getLE16(framePtr + 6));

		if (frameHeader.type == FrameType::FRAME_TYPE)
		{
			// Check each chunk's type and decode its data if relevant.
			uint32_t chunkOffset = sizeof(FrameHeader);
			for (uint16_t i = 0; i < frameHeader.chunkCount; ++i)
			{
				// Pointer to the chunk's header.
				const uint8_t *chunkPtr = framePtr + chunkOffset;

				const ChunkHeader chunkHeader(Bytes::getLE32(chunkPtr),
					Bytes::getLE16(chunkPtr + 4));

				// The struct alignment of 8 means sizeof(ChunkHeader) wouldn't
				// be accurate here, so 6 is used instead.
				const uint8_t *chunkData = chunkPtr + 6;

				// Just concerned with palettes, full frames, and delta frames.
				if (chunkHeader.type == ChunkType::COLOR_256)
				{
					// Palette chunk.
					this->readPaletteData(chunkData, palette);
				}
				else if (chunkHeader.type == ChunkType::FLI_BRUN)
				{
					// Full frame chunk.
					auto frame = this->decodeFullFrame(chunkData, chunkHeader.size,
						palette, framePixels);

					this->pixels.push_back(std::move(frame));
				}
				else if (chunkHeader.type == ChunkType::FLI_SS2)
				{
					// Delta frame chunk.
					auto frame = this->decodeDeltaFrame(chunkData, chunkHeader.size,
						palette, framePixels);

					this->pixels.push_back(std::move(frame));
				}
				else
				{
					// Ignoring other chunk types for now since they're not needed.
					/*Debug::mention("FLCFile", "Unrecognized chunk type \"" +
						std::to_string(static_cast<int>(chunkHeader.type)) + "\".");*/
				}

				chunkOffset += chunkHeader.size;
			}
		}
		else if (frameHeader.type == FrameType::PREFIX_CHUNK)
		{
			// CEL prefix chunk, can be skipped.
		}
		else
		{
			DebugCrash("Unrecognized frame type \"" +
				std::to_string(static_cast<int>(frameHeader.type)) + "\".");
		}

		dataOffset += frameHeader.size;
	}

	// Pop the last frame off, since they all seem to loop around to the beginning
	// at the end.
	this->pixels.pop_back();
}

FLCFile::~FLCFile()
{

}

void FLCFile::readPaletteData(const uint8_t *chunkData, Palette &dstPalette)
{
	// The number of elements (i.e., "groups" of pixels) should be one.
	const uint16_t numberOfElements = Bytes::getLE16(chunkData);
	DebugAssert(numberOfElements == 1, "Unusual palette element count: " + 
		std::to_string(numberOfElements) + ".");

	// Skip count and color count should both be ignored (one byte each).

	// Read through the RGB components and place them in the palette. There isn't 
	// a need for the first color to be transparent.
	const uint8_t *colorData = chunkData + 4;
	for (int i = 0; i < 255; ++i)
	{
		const uint8_t *ptr = colorData + (i * 3);
		const uint8_t r = *(ptr + 0);
		const uint8_t g = *(ptr + 1);
		const uint8_t b = *(ptr + 2);
		dstPalette.get()[i] = Color(r, g, b, 255);
	}
}

std::unique_ptr<uint32_t[]> FLCFile::decodeFullFrame(const uint8_t *chunkData,
	int chunkSize, const Palette &palette, std::vector<uint8_t> &initialFrame)
{
	// Decode a fullscreen image chunk. Most likely the first image in the FLIC.
	std::vector<uint8_t> decomp(this->width * this->height);

	// The chunk data is organized in rows, and each row has packets of compressed
	// pixels. The number of lines is the height of the FLIC.
	const int lineCount = this->height;

	int offset = 0;
	for (int rowsDone = 0; rowsDone < lineCount; ++rowsDone)
	{
		// The first byte of each line is the ignored packet count. The total width 
		// of the line after decoding pixels is used instead.
		offset++;

		// Read and process packets until the pixel count for the row is equal to 
		// the width.
		int rowPixelsDone = 0;
		while (rowPixelsDone < this->width)
		{
			// The meaning of "type" depends on its sign.
			const int8_t type = *(chunkData + offset);

			if (type > 0)
			{
				// The packet contains one pixel that is repeated by the absolute 
				// value of "type". This is probably used frequently for black pixels.
				const uint8_t pixel = *(chunkData + offset + 1);

				for (int i = 0; i < type; ++i)
				{
					decomp.at((rowPixelsDone + i) + (rowsDone * this->width)) = pixel;
				}

				rowPixelsDone += type;
				offset += 2;
			}
			else if (type < 0)
			{
				// "Type" is a pixel count for how many to copy from the packet 
				// to the output.
				const int8_t pixelCount = -type;

				for (int i = 0; i < pixelCount; ++i)
				{
					const uint8_t pixel = *(chunkData + offset + 1 + i);
					decomp.at((rowPixelsDone + i) + (rowsDone * this->width)) = pixel;
				}

				rowPixelsDone += pixelCount;
				offset += 1 + pixelCount;
			}
			else
			{
				DebugCrash("Byte run error (packet cannot be zero).");
			}
		}
	}

	// Write the decoded frame to the initial (scratch) frame.
	initialFrame = decomp;

	const uint8_t *decompPixels = decomp.data();
	std::unique_ptr<uint32_t[]> image(new uint32_t[this->width * this->height]);

	std::transform(decompPixels, decompPixels + decomp.size(), image.get(),
		[&palette](uint8_t col) -> uint32_t
	{
		return palette.get()[col].toARGB();
	});

	return std::move(image);
}

std::unique_ptr<uint32_t[]> FLCFile::decodeDeltaFrame(const uint8_t *chunkData,
	int chunkSize, const Palette &palette, std::vector<uint8_t> &initialFrame)
{
	// Decode a delta frame chunk. The majority of FLIC frames are this format.

	// The line count is the number of rows with encoded packets.
	const uint16_t lineCount = Bytes::getLE16(chunkData);

	// Current row.
	int y = 0;

	// Byte offset in chunkData.
	int offset = 2;

	for (int linesDone = 0; linesDone < lineCount; ++y, ++linesDone)
	{
		// The packet count is obtained from a packet whose two most significant 
		// bits are zero.
		int packetCount = 0;

		// Walk through the data until a non-negative packet is found.
		while (offset < chunkSize)
		{
			const int16_t packet = Bytes::getLE16(chunkData + offset);
			offset += 2;

			// Check if the two most significant bits are set.
			const bool bit15 = (packet & 0x8000) != 0;
			const bool bit14 = (packet & 0x4000) != 0;

			if (bit15)
			{
				if (bit14)
				{
					// Bit 15 and 14 are set. Skip some rows.
					const int16_t skipCount = -packet;
					y += skipCount;
				}
				else
				{
					// Bit 15 (the sign bit) is set. Set the last pixel in the row using
					// the lower byte of the packet.
					const uint8_t pixel = packet & 0x00FF;
					initialFrame.at((this->width - 1) + (y * this->width)) = pixel;

					// Go to the next row.
					y++;
				}
			}
			else
			{
				// Bit 15 and 14 are both zero. Use the packet's value as the count.
				packetCount = packet;
				break;
			}
		}

		// Current column in the row.
		int x = 0;

		// A packet with a non-negative value was found. Decode the following bytes
		// and write their values to the output buffer.
		for (int i = 0; i < packetCount; ++i)
		{
			// The first byte is the column skip count.
			x += *(chunkData + offset);

			// The second byte is the type (or count).
			const int8_t count = *(chunkData + offset + 1);
			offset += 2;

			// The sign of "count" determines how the next few bytes are interpreted.
			if (count > 0)
			{
				// Read "count" * 2 colors and write them to the output frame.
				for (int i = 0; (i < count) && (x < this->width); ++i)
				{
					const uint8_t color1 = *(chunkData + offset);
					const uint8_t color2 = *(chunkData + offset + 1);

					initialFrame.at(x + (y * this->width)) = color1;
					x++;

					if (x < this->width)
					{
						initialFrame.at(x + (y * this->width)) = color2;
						x++;
					}

					offset += 2;
				}
			}
			else if (count < 0)
			{
				// Read two colors and duplicate them "count" times.
				const uint8_t color1 = *(chunkData + offset);
				const uint8_t color2 = *(chunkData + offset + 1);

				// Reverse the sign of count so it's positive.
				const int8_t positiveCount = -count;

				for (int i = 0; (i < positiveCount) && (x < this->width); ++i)
				{
					initialFrame.at(x + (y * this->width)) = color1;
					x++;

					if (x < this->width)
					{
						initialFrame.at(x + (y * this->width)) = color2;
						x++;
					}
				}

				offset += 2;
			}
			else
			{
				DebugCrash("Delta packet type cannot be zero.");
			}
		}
	}

	// Use the modified initial frame as the source instead of a separate
	// decompressed buffer.
	const uint8_t *framePixels = initialFrame.data();
	std::unique_ptr<uint32_t[]> image(new uint32_t[this->width * this->height]);

	std::transform(framePixels, framePixels + initialFrame.size(), image.get(),
		[&palette](uint8_t col) -> uint32_t
	{
		return palette.get()[col].toARGB();
	});

	return std::move(image);
}

int FLCFile::getFrameCount() const
{
	return static_cast<int>(this->pixels.size());
}

double FLCFile::getFrameDuration() const
{
	return this->frameDuration;
}

int FLCFile::getWidth() const
{
	return this->width;
}

int FLCFile::getHeight() const
{
	return this->height;
}

uint32_t *FLCFile::getPixels(int index) const
{
	return this->pixels.at(index).get();
}
