#ifndef TXT_FILE_H
#define TXT_FILE_H

#include <cstdint>

#include "components/utilities/Buffer2D.h"

// Despite being called a .TXT file, this is a texture format used only in one place (FOG.TXT) for the
// screen-space fog effect.

class TXTFile
{
private:
	Buffer2D<uint16_t> pixels;
public:
	static constexpr int WIDTH = 128;
	static constexpr int HEIGHT = WIDTH;

	// The value to divide a pixel by to get its intensity as a percentage.
	static constexpr double PIXEL_DIVISOR = 4096.0;

	bool init(const char *filename);

	const uint16_t *getPixels() const;
};

#endif
