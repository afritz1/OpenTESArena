#include <algorithm>

#include "TXTFile.h"

#include "components/debug/Debug.h"
#include "components/vfs/manager.hpp"

bool TXTFile::init(const char *filename)
{
    Buffer<std::byte> src;
    if (!VFS::Manager::get().read(filename, &src))
    {
        DebugLogError("Could not open \"" + std::string(filename) + "\".");
        return false;
    }

    const int srcPixelCount = src.getCount() / 2;
    if (srcPixelCount != (TXTFile::WIDTH * TXTFile::HEIGHT))
    {
        DebugLogError("Invalid .TXT file pixel count (" + std::to_string(srcPixelCount) + "), needs to be " +
            std::to_string(TXTFile::WIDTH) + "x" + std::to_string(TXTFile::HEIGHT) + ".");
        return false;
    }

    const uint16_t *srcPixels = reinterpret_cast<const uint16_t*>(src.begin());

    this->pixels.init(TXTFile::WIDTH, TXTFile::HEIGHT);
    std::copy(srcPixels, srcPixels + srcPixelCount, this->pixels.begin());

    return true;
}

const uint16_t *TXTFile::getPixels() const
{
	return this->pixels.begin();
}
