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
	header.reserved1 = Bytes::getLE16(srcData.data() + 20);
	header.created = Bytes::getLE32(srcData.data() + 22);
	header.creator = Bytes::getLE32(srcData.data() + 26);
	header.updated = Bytes::getLE32(srcData.data() + 30);
	header.updater = Bytes::getLE32(srcData.data() + 34);
	header.aspect_dx = Bytes::getLE16(srcData.data() + 38);
	header.aspect_dy = Bytes::getLE16(srcData.data() + 40);
	header.ext_flags = Bytes::getLE16(srcData.data() + 42);
	header.keyframes = Bytes::getLE16(srcData.data() + 44);
	header.totalframes = Bytes::getLE16(srcData.data() + 46);
	header.req_memory = Bytes::getLE32(srcData.data() + 48);
	header.max_regions = Bytes::getLE16(srcData.data() + 52);
	header.transp_num = Bytes::getLE16(srcData.data() + 54);
	//header.reserved2; // 24 bytes, set to zero.
	header.oframe1 = Bytes::getLE32(srcData.data() + 80);
	header.oframe2 = Bytes::getLE32(srcData.data() + 84);

	Debug::mention("FLCFile", "\"" + filename + "\" header data:\n" +
		"- Size: " + std::to_string(header.size) + "\n" +
		"- Type: " + std::to_string(header.type) + "\n" +
		"- Frames: " + std::to_string(header.frames) + "\n" +
		"- Width: " + std::to_string(header.width) + "\n" +
		"- Height: " + std::to_string(header.height) + "\n" +
		"- Depth: " + std::to_string(header.depth) + "\n" +
		"- Flags: " + std::to_string(header.flags) + "\n" +
		"- Speed: " + std::to_string(header.speed) + "\n" +
		"- Created: " + std::to_string(header.created) + "\n" +
		"- Creator: " + std::to_string(header.creator) + "\n" +
		"- Updated: " + std::to_string(header.updated) + "\n" +
		"- Updater: " + std::to_string(header.updater) + "\n" +
		"- Aspect dx: " + std::to_string(header.aspect_dx) + "\n" +
		"- Aspect dy: " + std::to_string(header.aspect_dy) + "\n" +
		"- Extension flags: " + std::to_string(header.ext_flags) + "\n" +
		"- Key frames: " + std::to_string(header.keyframes) + "\n" +
		"- Total frames: " + std::to_string(header.totalframes) + "\n" +
		"- Required memory: " + std::to_string(header.req_memory) + "\n" +
		"- Max regions: " + std::to_string(header.max_regions) + "\n" +
		"- Transparent number: " + std::to_string(header.transp_num) + "\n" +
		"- Offset 1: " + std::to_string(header.oframe1) + "\n" +
		"- Offset 2: " + std::to_string(header.oframe2));

	this->width = header.width;
	this->height = header.height;

	Debug::crash("FLCFile", "Not implemented.");
}

FLCFile::~FLCFile()
{

}

int FLCFile::getFrameCount() const
{
	return static_cast<int>(this->pixels.size());
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
