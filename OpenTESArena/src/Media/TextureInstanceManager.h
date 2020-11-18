#ifndef TEXTURE_INSTANCE_MANAGER_H
#define TEXTURE_INSTANCE_MANAGER_H

#include <unordered_map>
#include <vector>

#include "Image.h"
#include "Palette.h"
#include "TextureUtils.h"
#include "../Interface/Surface.h"
#include "../Interface/Texture.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferRef.h"
#include "components/utilities/BufferRef2D.h"

// Similar to TextureManager but for textures created in-engine, not loaded from file.
// Intended to keep code from managing texture lifetimes themselves and to allow sharing.

class Renderer;

// BufferRef variations for avoiding returning easily-stale handles from texture instance manager.
// All references are read-only interfaces.
using ImageInstanceRef = BufferRef2D<const std::vector<Image>, const Image>;
using SurfaceInstanceRef = BufferRef2D<const std::vector<Surface>, const Surface>;
using TextureInstanceRef = BufferRef2D<const std::vector<Texture>, const Texture>;

class TextureInstanceManager
{
private:
	// All textures are reference-counted for ease of use, so users don't need to worry about
	// freeing when multiple things share the same ID.
	std::vector<Image> images;
	std::vector<Surface> surfaces;
	std::vector<Texture> textures;

	std::unordered_map<ImageInstanceID, int> imageRefCounts;
	std::unordered_map<SurfaceInstanceID, int> surfaceRefCounts;
	std::unordered_map<TextureInstanceID, int> textureRefCounts;

	std::vector<ImageInstanceID> freeImageIDs;
	std::vector<SurfaceInstanceID> freeSurfaceIDs;
	std::vector<TextureInstanceID> freeTextureIDs;

	ImageInstanceID getNextFreeImageID();
	SurfaceInstanceID getNextFreeSurfaceID();
	TextureInstanceID getNextFreeTextureID();
public:
	static constexpr int NO_ID = -1;

	// @todo: makeSurfaceFrom8Bit() from TextureManager?

	// Texture creation functions.
	ImageInstanceID makeImage(int width, int height, const PaletteID *paletteID = nullptr);
	SurfaceInstanceID makeSurface(int width, int height);
	TextureInstanceID makeTexture(int width, int height, Renderer &renderer);

	// Returns texture reference wrappers, protecting from dangling pointers.
	ImageInstanceRef getImageRef(ImageInstanceID id) const;
	SurfaceInstanceRef getSurfaceRef(SurfaceInstanceID id) const;
	TextureInstanceRef getTextureRef(TextureInstanceID id) const;

	// Returns raw texture handles, not protected from dangling pointers.
	Image &getImageHandle(ImageInstanceID id);
	const Image &getImageHandle(ImageInstanceID id) const;
	Surface &getSurfaceHandle(SurfaceInstanceID id);
	const Surface &getSurfaceHandle(SurfaceInstanceID id) const;
	Texture &getTextureHandle(TextureInstanceID id);
	const Texture &getTextureHandle(TextureInstanceID id) const;

	bool tryIncrementImageRefCount(ImageInstanceID id);
	bool tryIncrementSurfaceRefCount(SurfaceInstanceID id);
	bool tryIncrementTextureRefCount(TextureInstanceID id);

	void decrementImageRefCount(ImageInstanceID id);
	void decrementSurfaceRefCount(SurfaceInstanceID id);
	void decrementTextureRefCount(TextureInstanceID id);

	void clear();
};

#endif
