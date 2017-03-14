#include <algorithm>

#include "SETFile.h"

#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

const int SETFile::CHUNK_WIDTH = 64;
const int SETFile::CHUNK_HEIGHT = 64;
const int SETFile::CHUNK_SIZE = SETFile::CHUNK_WIDTH * SETFile::CHUNK_HEIGHT;

SETFile::SETFile(const std::string &filename, const Palette &palette)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "SETFile", "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Number of uncompressed chunks packed in the SET.
	const int chunkCount = static_cast<int>(fileSize) / SETFile::CHUNK_SIZE;

	// Create an image for each uncompressed chunk using the given palette.
	for (int chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
	{
		this->chunks.push_back(std::unique_ptr<uint32_t[]>(
			new uint32_t[SETFile::CHUNK_SIZE]));

		const int byteOffset = SETFile::CHUNK_SIZE * chunkIndex;

		std::transform(srcData.begin() + byteOffset,
			srcData.begin() + byteOffset + SETFile::CHUNK_SIZE,
			this->chunks.at(chunkIndex).get(),
			[&palette](uint8_t col) -> uint32_t
		{
			return palette.get()[col].toARGB();
		});
	}
}

SETFile::~SETFile()
{

}

int SETFile::getImageCount() const
{
	return static_cast<int>(this->chunks.size());
}

uint32_t *SETFile::getPixels(int index) const
{
	return this->chunks.at(index).get();
}
