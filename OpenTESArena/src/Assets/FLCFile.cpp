#include <algorithm>
#include <array>

#include "FLCFile.h"

#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

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
	std::array<uint8_t, 24> reserved2; // Set to zero.
	uint32_t oframe1;       // Offset to frame 1 (FLC only).
	uint32_t oframe2;       // Offset to frame 2 (FLC only).
	std::array<uint8_t, 40> reserved3; // Set to zero.
};

enum class FileType
{
	FLC_TYPE = 0xAF12
};

enum class ChunkType
{
	COLOR_256 = 0x04,
	FLI_SS2 = 0x07, // or DELTA_FLC.
	COLOR_64 = 0x0B,
	FLI_LC = 0x0C, // or DELTA_FLI.
	BLACK = 0x0D,
	FLI_BRUN = 0x0F, // or BYTE_RUN.
	FLI_COPY = 0x10,
	EIGHTEEN = 0x12
};

enum class FrameType
{
	PREFIX_CHUNK = 0xF100,
	FRAME_TYPE = 0xF1FA
};

FLCFile::FLCFile(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "FLCFile", "Could not open \"" + filename + "\".");

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
	Debug::check(header.type == static_cast<int>(FileType::FLC_TYPE), "FLCFile",
		"Unsupported type \"" + std::to_string(header.type) + "\".");

	this->frameDuration = static_cast<double>(header.speed) / 1000.0;
	this->width = header.width;
	this->height = header.height;

	// Palette which is constructed by one of the FLC palette chunks.
	Palette palette;

	// Current state of the frame's palette indices. Completely updated by byte runs
	// and partially updated by delta frames.
	std::vector<uint8_t> framePixels(this->width * this->height);
	std::fill(framePixels.begin(), framePixels.end(), 0);

	// Start decoding frames. The data starts after the header (at 128, not sizeof(FLICHeader)?).
	uint32_t dataOffset = 128;
	while ((srcData.begin() + dataOffset) < srcData.end())
	{
		const uint8_t *frameHeader = srcData.data() + dataOffset;
		const uint32_t frameSize = Bytes::getLE32(frameHeader);
		const FrameType frameType = static_cast<FrameType>(Bytes::getLE16(frameHeader + 4));

		if (frameType == FrameType::FRAME_TYPE)
		{
			// Get the number of chunks in the frame. 
			// Ignore the delay override at frameData + 8.
			const uint16_t frameChunks = Bytes::getLE16(frameHeader + 6);

			// Check each chunk's frame type and decode its data if relevant.
			uint32_t chunkOffset = 16;
			for (uint16_t i = 0; i < frameChunks; ++i)
			{
				const uint8_t *chunkHeader = frameHeader + chunkOffset;
				const uint32_t chunkSize = Bytes::getLE32(chunkHeader);
				const ChunkType chunkType = static_cast<ChunkType>(Bytes::getLE16(chunkHeader + 4));
				const uint8_t *chunkData = chunkHeader + 6;

				// Just concerned with palettes, full frames, and delta frames.
				if (chunkType == ChunkType::COLOR_256)
				{
					// Palette chunk.
					this->readPaletteData(chunkData, palette);
				}
				else if (chunkType == ChunkType::FLI_BRUN)
				{
					// Full frame chunk.
					auto frame = this->decodeFullFrame(chunkData, chunkSize, palette, framePixels);
					this->pixels.push_back(std::move(frame));
				}
				else if (chunkType == ChunkType::FLI_SS2)
				{
					// Delta frame chunk.
					auto frame = this->decodeDeltaFrame(chunkData, chunkSize, palette, framePixels);
					this->pixels.push_back(std::move(frame));
				}

				chunkOffset += chunkSize;
			}
		}
		else if (frameType == FrameType::PREFIX_CHUNK)
		{
			// Prefix chunk (associated with .CEL files).
		}
		else
		{
			Debug::crash("FLCFile", "Unrecognized frame type \"" +
				std::to_string(static_cast<int>(frameType)) + "\".");
		}

		dataOffset += frameSize;
	}

	// The frame decoding functionality is unfinished for now, but the interface with
	// the program is done. That means the program can act like the videos are playing 
	// correctly.
	//Debug::crash("FLCFile", "Not implemented.");
}

FLCFile::~FLCFile()
{

}

void FLCFile::readPaletteData(const uint8_t *chunkData, Palette &dstPalette)
{
	const uint8_t *colorPtr = chunkData + 2;

	// For simplicity, assume there is one color packet, and it is 768 bytes long.
	// This may or may not work for all FLC files.
	for (int i = 0; i < 255; ++i)
	{
		const uint8_t *ptr = colorPtr + (i * 3);
		const uint8_t r = *(ptr + 0);
		const uint8_t g = *(ptr + 1);
		const uint8_t b = *(ptr + 2);
		dstPalette.at(i) = Color(r, g, b);
	}
}

std::unique_ptr<uint32_t> FLCFile::decodeFullFrame(const uint8_t *chunkData,
	int chunkSize, const Palette &palette, std::vector<uint8_t> &initialFrame)
{
	// Decode a fullscreen image chunk. Most likely the first image in the FLC.
	std::vector<uint8_t> decomp(this->width * this->height);
	
	// Cannot use Compression::decodeRLE() because FLCs use a different RLE method.
	// Maybe have a vector of vectors indicating each RLE-decompressed line of pixels?
	// To do: use actual byte decompression here.
	std::copy(chunkData, chunkData + chunkSize, decomp.data());

	// Write to initial frame...?

	const uint8_t *decompPixels = decomp.data();
	std::unique_ptr<uint32_t> image(new uint32_t[this->width * this->height]);

	std::transform(decompPixels, decompPixels + decomp.size(), image.get(),
		[&palette](uint8_t col) -> uint32_t
	{
		return palette[col].toARGB();
	});

	return std::move(image);
}

std::unique_ptr<uint32_t> FLCFile::decodeDeltaFrame(const uint8_t *chunkData,
	int chunkSize, const Palette &palette, std::vector<uint8_t> &initialFrame)
{
	// Decode a delta frame chunk. The majority of FLC frames are this format.
	std::vector<uint8_t> decomp(this->width * this->height);

	// Cannot use Compression::decodeRLE() because FLCs use a different RLE method.
	// Maybe have a vector of vectors indicating each RLE-decompressed line of pixels?
	// To do: use actual byte decompression here.
	std::copy(chunkData, chunkData + chunkSize, decomp.data());

	// Write to initial frame...?

	const uint8_t *decompPixels = decomp.data();
	std::unique_ptr<uint32_t> image(new uint32_t[this->width * this->height]);

	std::transform(decompPixels, decompPixels + decomp.size(), image.get(),
		[&palette](uint8_t col) -> uint32_t
	{
		return palette[col].toARGB();
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
