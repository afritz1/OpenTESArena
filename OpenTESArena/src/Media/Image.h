#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <optional>

#include "Color.h"
#include "Palette.h"
#include "TextureUtils.h"

#include "components/utilities/Buffer2D.h"

// 8-bit 2D buffer with optional palette.

class Image
{
private:
	Buffer2D<uint8_t> pixels;
	std::optional<PaletteID> paletteID;
public:
	void init(int width, int height, const std::optional<PaletteID> &paletteID);

	int getWidth() const;
	int getHeight() const;
	uint8_t *getPixels();
	const uint8_t *getPixels() const;
	PaletteID *getPaletteID();
	const PaletteID *getPaletteID() const;

	uint8_t getPixel(int x, int y) const;
	void setPixel(int x, int y, uint8_t color);

	void clear();
};

#endif
