#ifndef TEXTURE_BUILDER_H
#define TEXTURE_BUILDER_H

#include <cstdint>

#include "components/utilities/Buffer2D.h"

enum class TextureBuilderType
{
	Paletted,
	TrueColor
};

// Intermediate texture data for initializing renderer-specific textures (voxels, entities, UI, etc.).
class TextureBuilder
{
public:
	struct PalettedTexture
	{
		Buffer2D<uint8_t> texels;

		void init(int width, int height, const uint8_t *texels);
	};

	struct TrueColorTexture
	{
		Buffer2D<uint32_t> texels;

		void init(int width, int height, const uint32_t *texels);
	};
private:
	TextureBuilderType type;
	PalettedTexture paletteTexture;
	TrueColorTexture trueColorTexture;
public:
	TextureBuilder();

	void initPaletted(int width, int height, const uint8_t *texels);
	void initTrueColor(int width, int height, const uint32_t *texels);

	int getWidth() const;
	int getHeight() const;
	int getBytesPerTexel() const;
	TextureBuilderType getType() const;
	const PalettedTexture &getPaletted() const;
	const TrueColorTexture &getTrueColor() const;
};

#endif
