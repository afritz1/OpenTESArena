#ifndef RENDER_TEXTURE_ALLOCATOR_H
#define RENDER_TEXTURE_ALLOCATOR_H

#include <cstdint>

#include "../Assets/TextureUtils.h"
#include "../Utilities/Palette.h"

#include "components/utilities/Span2D.h"

struct TextureBuilder;

// Allocators provided by render backend to abstract which memory they occupy (RAM or VRAM).
struct ObjectTextureAllocator
{
	virtual ObjectTextureID create(int width, int height, int bytesPerTexel) = 0;
	virtual ObjectTextureID create(const TextureBuilder &textureBuilder) = 0;
	virtual void free(ObjectTextureID textureID) = 0;

	virtual LockedTexture lock(ObjectTextureID textureID) = 0;
	virtual void unlock(ObjectTextureID textureID) = 0;
};

struct UiTextureAllocator
{
	virtual UiTextureID create(int width, int height) = 0;
	virtual UiTextureID create(Span2D<const uint32_t> texels) = 0;
	virtual UiTextureID create(Span2D<const uint8_t> texels, const Palette &palette) = 0;
	virtual UiTextureID create(TextureBuilderID textureBuilderID, PaletteID paletteID, const TextureManager &textureManager) = 0;

	virtual void free(UiTextureID textureID) = 0;

	virtual LockedTexture lock(UiTextureID textureID) = 0;
	virtual void unlock(UiTextureID textureID) = 0;
};

#endif
