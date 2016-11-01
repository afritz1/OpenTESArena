#include <algorithm>

#include "RCIFile.h"

#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

const int RCIFile::FRAME_WIDTH = 320;
const int RCIFile::FRAME_HEIGHT = 100;
const int RCIFile::FRAME_SIZE = RCIFile::FRAME_WIDTH * RCIFile::FRAME_HEIGHT;

RCIFile::RCIFile(const std::string &filename, const Palette &palette)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "RCIFile", "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Number of uncompressed frames packed in the RCI.
	const int frameCount = static_cast<int>(fileSize) / RCIFile::FRAME_SIZE;

	// Create an image for each uncompressed frame using the given palette.
	for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex)
	{
		this->frames.push_back(std::unique_ptr<uint32_t>(
			new uint32_t[RCIFile::FRAME_SIZE]));

		const int byteOffset = RCIFile::FRAME_SIZE * frameIndex;

		std::transform(srcData.begin() + byteOffset,
			srcData.begin() + byteOffset + RCIFile::FRAME_SIZE,
			this->frames.at(frameIndex).get(),
			[&palette](uint8_t col) -> uint32_t
		{
			return palette[col].toARGB();
		});
	}
}

RCIFile::~RCIFile()
{

}

int RCIFile::getCount() const
{
	return static_cast<int>(this->frames.size());
}

uint32_t *RCIFile::getPixels(int index) const
{
	return this->frames.at(index).get();
}
