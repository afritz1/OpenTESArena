#ifndef RENDER_TEXTURE_UTILS_H
#define RENDER_TEXTURE_UTILS_H

// Common texture handles allocated by a renderer for a user when they want a new texture in the
// internal renderer format.

using VoxelTextureID = int; // Only for voxels, must be power-of-2 for mipmaps.
using EntityTextureID = int; // One per frame of entity animations, any dimensions.
using SkyTextureID = int; // Similar to entity textures but for mountains/clouds/stars/etc.
using UiTextureID = int; // Used with all UI textures.

class RendererInterface;

// Convenience classes for creating and automatically destroying a texture.
class ScopedVoxelTextureRef
{
private:
	VoxelTextureID id;
	RendererInterface *rendererInterface;
public:
	ScopedVoxelTextureRef(VoxelTextureID id, RendererInterface &rendererInterface);
	~ScopedVoxelTextureRef();

	VoxelTextureID get() const;
};

class ScopedEntityTextureRef
{
private:
	EntityTextureID id;
	RendererInterface *rendererInterface;
public:
	ScopedEntityTextureRef(EntityTextureID id, RendererInterface &rendererInterface);
	~ScopedEntityTextureRef();

	EntityTextureID get() const;
};

class ScopedSkyTextureRef
{
private:
	SkyTextureID id;
	RendererInterface *rendererInterface;
public:
	ScopedSkyTextureRef(SkyTextureID id, RendererInterface &rendererInterface);
	~ScopedSkyTextureRef();

	SkyTextureID get() const;
};

class ScopedUiTextureRef
{
private:
	UiTextureID id;
	RendererInterface *rendererInterface;
public:
	ScopedUiTextureRef(UiTextureID id, RendererInterface &rendererInterface);
	~ScopedUiTextureRef();

	UiTextureID get() const;
};

#endif
