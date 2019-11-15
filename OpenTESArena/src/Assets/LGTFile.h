#ifndef LGT_FILE_H
#define LGT_FILE_H

#include <cstdint>

#include "components/utilities/Buffer2D.h"

// Light level file, contains 13 light palettes for shading/transparencies.

// In some foggy dungeons, the game seems to use fog distance for determining
// light level (FOG.LGT).

class LGTFile
{
private:
	Buffer2D<uint8_t> palettes;
public:
	static const int PALETTE_COUNT;
	static const int ELEMENTS_PER_PALETTE;

	bool init(const char *filename);

	const uint8_t *getPalette(int index) const;
};

#endif
