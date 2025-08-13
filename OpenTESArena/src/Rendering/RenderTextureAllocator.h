#ifndef RENDER_TEXTURE_ALLOCATOR_H
#define RENDER_TEXTURE_ALLOCATOR_H

#include <optional>

#include "../Assets/TextureUtils.h"
#include "../Math/Vector2.h"

// Allocators provided by render backend to abstract which memory they occupy (RAM or VRAM).
struct ObjectTextureAllocator
{
	virtual ObjectTextureID create(int width, int height, int bytesPerTexel) = 0;
	virtual void free(ObjectTextureID textureID) = 0;

	virtual std::optional<Int2> tryGetDimensions(ObjectTextureID id) const = 0;

	virtual LockedTexture lock(ObjectTextureID textureID) = 0;
	virtual void unlock(ObjectTextureID textureID) = 0;
};

struct UiTextureAllocator
{
	virtual UiTextureID create(int width, int height) = 0;
	virtual void free(UiTextureID textureID) = 0;

	virtual std::optional<Int2> tryGetDimensions(UiTextureID id) const = 0;

	virtual LockedTexture lock(UiTextureID textureID) = 0;
	virtual void unlock(UiTextureID textureID) = 0;
};

#endif
