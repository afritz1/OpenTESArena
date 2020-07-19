#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <memory>

#include "Color.h"
#include "Palette.h"

#include "components/utilities/Buffer2D.h"

// 8-bit 2D buffer with optional palette.

class Image
{
private:
	Buffer2D<uint8_t> pixels;
	std::unique_ptr<Palette> palette;
public:
	void init(int width, int height, const Palette *palette);

	int getWidth() const;
	int getHeight() const;
	uint8_t *getPixels();
	const uint8_t *getPixels() const;
	Palette *getPalette();
	const Palette *getPalette() const;

	uint8_t getPixel(int x, int y) const;
	void setPixel(int x, int y, uint8_t color);
};

#endif
