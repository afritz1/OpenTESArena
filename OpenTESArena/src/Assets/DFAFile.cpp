#include <algorithm>

#include "DFAFile.h"

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

	// To do...

	Debug::crash("DFAFile", "Not implemented.");
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
