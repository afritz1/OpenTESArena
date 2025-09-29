#ifndef TEXTURE_BUILDER_H
#define TEXTURE_BUILDER_H

#include <cstddef>
#include <cstdint>

#include "components/utilities/Buffer.h"
#include "components/utilities/Span2D.h"

// Intermediate texture for initializing other renderer-specific textures for the game world or UI.
struct TextureBuilder
{
	Buffer<std::byte> bytes;
	int width;
	int height;
	int bytesPerTexel;

	TextureBuilder();

	void initPaletted(int width, int height, const uint8_t *texels);
	void initHighColor(int width, int height, const uint16_t *texels);
	void initTrueColor(int width, int height, const uint32_t *texels);

	Span2D<const uint8_t> getTexels8() const;
	Span2D<const uint16_t> getTexels16() const;
	Span2D<const uint32_t> getTexels32() const;
};

#endif
