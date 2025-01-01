#ifndef TEXTURE_BUILDER_H
#define TEXTURE_BUILDER_H

#include <cstdint>

#include "components/utilities/Buffer2D.h"

enum class TextureBuilderType
{
	Paletted,
	TrueColor
};

struct TextureBuilderPalettedTexture
{
	Buffer2D<uint8_t> texels;

	void init(int width, int height, const uint8_t *texels);
};

struct TextureBuilderTrueColorTexture
{
	Buffer2D<uint32_t> texels;

	void init(int width, int height, const uint32_t *texels);
};

// Intermediate texture data for initializing renderer-specific textures (voxels, entities, UI, etc.).
struct TextureBuilder
{
	TextureBuilderType type;
	TextureBuilderPalettedTexture paletteTexture;
	TextureBuilderTrueColorTexture trueColorTexture;

	TextureBuilder();

	void initPaletted(int width, int height, const uint8_t *texels);
	void initTrueColor(int width, int height, const uint32_t *texels);

	int getWidth() const;
	int getHeight() const;
	int getBytesPerTexel() const;
};

#endif
