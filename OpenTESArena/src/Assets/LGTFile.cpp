#include <algorithm>

#include "LGTFile.h"

#include "components/debug/Debug.h"
#include "components/vfs/manager.hpp"

bool LGTFile::init(const char *filename)
{
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.begin());

	constexpr int elementCount = LGTFile::ELEMENTS_PER_PALETTE * LGTFile::PALETTE_COUNT;
	const uint8_t *srcEnd = srcPtr + elementCount;

	// Each row is a palette.
	this->palettes.init(LGTFile::ELEMENTS_PER_PALETTE, LGTFile::PALETTE_COUNT);

	std::copy(srcPtr, srcEnd, this->palettes.begin());
	return true;
}

BufferView<const uint8_t> LGTFile::getLightPalette(int index) const
{
	DebugAssert(index < LGTFile::PALETTE_COUNT);
	const uint8_t *ptr = this->palettes.begin() + (index * this->palettes.getWidth());
	return BufferView(ptr, LGTFile::ELEMENTS_PER_PALETTE);
}

BufferView2D<const uint8_t> LGTFile::getAllLightPalettes() const
{
	return BufferView2D<const uint8_t>(this->palettes);
}
