#include <algorithm>

#include "CFAFile.h"

#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

CFAFile::CFAFile(const std::string &filename, const Palette &palette)
	: pixels()
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "CFAFile", "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// To do...

	Debug::crash("CFAFile", "Not implemented.");
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
