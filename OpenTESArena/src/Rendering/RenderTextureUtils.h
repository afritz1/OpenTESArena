#ifndef RENDER_TEXTURE_UTILS_H
#define RENDER_TEXTURE_UTILS_H

#include <cstddef>
#include <cstdint>

#include "RenderShaderUtils.h"

#include "../Math/Vector2.h"
#include "components/utilities/Span.h"
#include "components/utilities/Span2D.h"

// Handles to allocated textures in internal renderer format.
using ObjectTextureID = int; // For scene geometry (voxels/entities/sky/particles).
using UiTextureID = int; // For UI textures.

class Renderer;

struct LockedTexture
{
	Span<std::byte> texels;
	int width;
	int height;
	int bytesPerTexel;

	LockedTexture();
	LockedTexture(Span<std::byte> texels, int width, int height, int bytesPerTexel);

	bool isValid();

	Span2D<uint8_t> getTexels8();
	Span2D<uint32_t> getTexels32();
};

// Owning reference to an object texture ID.
class ScopedObjectTextureRef
{
private:
	ObjectTextureID id;
	Renderer *renderer;
	int width, height;

	void setDims();
public:
	ScopedObjectTextureRef(ObjectTextureID id, Renderer &renderer);
	ScopedObjectTextureRef();
	ScopedObjectTextureRef(ScopedObjectTextureRef &&other);
	~ScopedObjectTextureRef();

	ScopedObjectTextureRef &operator=(ScopedObjectTextureRef &&other);

	void init(ObjectTextureID id, Renderer &renderer);

	ObjectTextureID get() const;
	int getWidth() const;
	int getHeight() const;

	// Texture updating functions.
	LockedTexture lockTexels();
	void unlockTexels();

	void destroy();
};

// Owning reference to a UI texture ID.
class ScopedUiTextureRef
{
private:
	UiTextureID id;
	Renderer *renderer;
	int width, height;

	void setDims();
public:
	ScopedUiTextureRef(UiTextureID id, Renderer &renderer);
	ScopedUiTextureRef();
	ScopedUiTextureRef(ScopedUiTextureRef &&other);
	~ScopedUiTextureRef();

	ScopedUiTextureRef &operator=(ScopedUiTextureRef &&other);

	void init(UiTextureID id, Renderer &renderer);

	UiTextureID get() const;
	int getWidth() const;
	int getHeight() const;
	Int2 getDimensions() const;

	// Texture updating functions. The returned pointer allows for changing any texels in the texture.
	LockedTexture lockTexels();
	void unlockTexels();
};

#endif
