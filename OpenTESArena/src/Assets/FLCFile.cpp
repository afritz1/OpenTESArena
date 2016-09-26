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
	const int32_t srcLen = static_cast<int32_t>(srcData.size());
	stream->read(reinterpret_cast<char*>(srcData.data()), srcLen);

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

	Debug::mention("FLCFile", "\"" + filename + "\" header data:\n" +
		"- Size: " + std::to_string(header.size) + "\n" +
		"- Type: " + std::to_string(header.type) + "\n" +
		"- Frames: " + std::to_string(header.frames) + "\n" +
		"- Width: " + std::to_string(header.width) + "\n" +
		"- Height: " + std::to_string(header.height) + "\n" +
		"- Depth: " + std::to_string(header.depth) + "\n" +
		"- Flags: " + std::to_string(header.flags) + "\n" +
		"- Speed: " + std::to_string(header.speed));

	// This class will only support the format used by Arena (0xAF12) for now.
	Debug::check(header.type == static_cast<int>(FileType::FLC_TYPE), "FLCFile",
		"Unsupported type \"" + std::to_string(header.type) + "\".");

	this->frameDuration = static_cast<double>(header.speed) / 1000.0;
	this->width = header.width;
	this->height = header.height;

	// Start decoding frames. The data starts at byte 128.
	uint32_t frameDataOffset = 0;
	Palette palette;

	// Current state of the frame's palette indices. Completely updated by byte runs,
	// and partially updated by delta frames.
	std::vector<uint8_t> framePixels(this->width * this->height);
	std::fill(framePixels.begin(), framePixels.end(), 0);

	while (true)
	{
		const uint8_t *frameData = srcData.data() + 128 + frameDataOffset;

		if ((frameData - srcData.data()) >= srcLen)
		{
			break;
		}

		const uint32_t frameSize = Bytes::getLE32(frameData);
		const uint16_t frameType = Bytes::getLE16(frameData + 4);

		Debug::mention("FLCFile", "Reading... frame size " + std::to_string(frameSize) +
			", frame type " + std::to_string(frameType));

		const bool isFrameType = frameType == static_cast<uint16_t>(FrameType::FRAME_TYPE);
		const bool isPrefixChunk = frameType == static_cast<uint16_t>(FrameType::PREFIX_CHUNK);

		if (isFrameType)
		{
			// Ignore the delay override (frameData + 8).
			const uint16_t frameChunks = Bytes::getLE16(frameData + 6);

			Debug::mention("FLCFile", "- Frame type. Frame chunks " +
				std::to_string(frameChunks));

			uint32_t chunkDataOffset = 0;

			// Find what type each chunk in the frame is and deal with them accordingly.
			for (uint16_t i = 0; i < frameChunks; ++i)
			{
				const uint8_t *chunkData = frameData + 16 + chunkDataOffset;
				const uint32_t chunkSize = Bytes::getLE32(chunkData);
				const uint16_t chunkType = Bytes::getLE16(chunkData + 4);

				if (chunkType == static_cast<uint16_t>(ChunkType::COLOR_256))
				{
					Debug::mention("FLCFile", "-> Color palette (256)");

					this->readPaletteData(chunkData + 6, palette);
				}
				else if (chunkType == static_cast<uint16_t>(ChunkType::FLI_SS2))
				{
					Debug::mention("FLCFile", "-> FLI_SS2 (DELTA_FLC)");
				}
				else if (chunkType == static_cast<uint16_t>(ChunkType::EIGHTEEN))
				{
					Debug::mention("FLCFile", "-> 18");
				}
				else if (chunkType == static_cast<uint16_t>(ChunkType::BLACK))
				{
					// No data in this chunk, so skip.
					Debug::mention("FLCFile", "-> Black");
				}
				else if (chunkType == static_cast<uint16_t>(ChunkType::FLI_BRUN))
				{
					Debug::mention("FLCFile", "-> FLI_BRUN (BYTE_RUN)");
				}
				else
				{
					Debug::crash("FLCFile", "Unrecognized chunk type \"" +
						std::to_string(chunkType) + "\".");
				}

				chunkDataOffset += chunkSize;
			}
		}
		else if (isPrefixChunk)
		{
			// For .CEL files?
			Debug::mention("FLCFile", "- Prefix chunk");
		}
		else
		{
			Debug::crash("FLCFile", "Unrecognized frame type \"" +
				std::to_string(frameType) + "\".");
		}

		frameDataOffset += frameSize;
	}

	Debug::crash("FLCFile", "Not implemented.");
}

FLCFile::~FLCFile()
{

}

void FLCFile::readPaletteData(const uint8_t *data, Palette &dstPalette)
{
	const uint8_t *colorPtr = data + 2;

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
	Debug::check((index >= 0) && (index < this->getFrameCount()), "FLCFile",
		"Frame index (" + std::to_string(index) + ") out of range.");

	return this->pixels[index].get();
}
