#include <algorithm>

#include "DFAFile.h"

#include "Compression.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

DFAFile::DFAFile(const std::string &filename, const Palette &palette)
	: pixels()
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "DFAFile", "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Read DFA header data.
	const uint16_t imageCount = Bytes::getLE16(srcData.data());
	const uint16_t unknown1 = Bytes::getLE16(srcData.data() + 2);
	const uint16_t unknown2 = Bytes::getLE16(srcData.data() + 4);
	const uint16_t width = Bytes::getLE16(srcData.data() + 6);
	const uint16_t height = Bytes::getLE16(srcData.data() + 8);
	const uint16_t compressedLength = Bytes::getLE16(srcData.data() + 10);

	this->width = width;
	this->height = height;

	// Uncompress the initial frame.
	std::vector<uint8_t> decomp(width * height);
	Compression::decodeRLE(srcData.data() + 12, width * height, decomp);

	// Write the pixel data to the first output frame.
	this->pixels.push_back(std::unique_ptr<uint32_t>(new uint32_t[width * height]));
	uint32_t *dstPixels = this->pixels.at(this->pixels.size() - 1).get();

	std::transform(decomp.begin(), decomp.end(), dstPixels,
		[&palette](uint8_t col) -> uint32_t
	{
		return palette[col].toARGB();
	});

	// To do... the rest of the animation blocks. Chunk data starts immediately
	// after the compressed pixels.

	//Debug::crash("DFAFile", "Not implemented.");
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
