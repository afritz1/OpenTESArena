#ifndef LGT_FILE_H
#define LGT_FILE_H

#include <cstdint>

#include "components/utilities/Buffer2D.h"
#include "components/utilities/BufferView.h"

// Light level file, contains 13 light palettes for shading/transparencies.

// In some foggy dungeons, the game seems to use fog distance for determining light level (FOG.LGT).

class LGTFile
{
private:
	Buffer2D<uint8_t> palettes;
public:
	static constexpr int PALETTE_COUNT = 13;
	static constexpr int ELEMENTS_PER_PALETTE = 256;

	bool init(const char *filename);

	BufferView<const uint8_t> getLightPalette(int index) const;
};

#endif
