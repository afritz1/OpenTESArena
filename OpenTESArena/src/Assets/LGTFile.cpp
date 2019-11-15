#include <algorithm>

#include "LGTFile.h"

#include "components/debug/Debug.h"
#include "components/vfs/manager.hpp"

const int LGTFile::PALETTE_COUNT = 13;
const int LGTFile::ELEMENTS_PER_PALETTE = 256;

bool LGTFile::init(const char *filename)
{
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.get());

	constexpr int elementCount = LGTFile::ELEMENTS_PER_PALETTE * LGTFile::PALETTE_COUNT;
	const uint8_t *srcEnd = srcPtr + elementCount;

	// Each row is a palette.
	this->palettes.init(LGTFile::ELEMENTS_PER_PALETTE, LGTFile::PALETTE_COUNT);

	std::copy(srcPtr, srcEnd, this->palettes.get());
	return true;
}

const uint8_t *LGTFile::getPalette(int index) const
{
	DebugAssert(index < LGTFile::PALETTE_COUNT);
	return this->palettes.get() + (index * LGTFile::ELEMENTS_PER_PALETTE);
}
