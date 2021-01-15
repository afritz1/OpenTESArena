#ifndef RENDER_TEXTURE_UTILS_H
#define RENDER_TEXTURE_UTILS_H

// Common texture handles allocated by a renderer for a user when they want a new texture in the
// internal renderer format.

using VoxelTextureID = int; // Only for voxels, must be power-of-2 for mipmaps.
using EntityTextureID = int; // One per frame of entity animations, any dimensions.
using SkyTextureID = int; // Similar to entity textures but for mountains/clouds/stars/etc.
using UiTextureID = int; // Used with all UI textures.

class RendererSystem2D;
class RendererSystem3D;

// Convenience classes for creating and automatically destroying a texture.
// @temp: commented out until the renderers are working with texture builders and texture IDs instead of
// TextureAssetReferences for texture handle creation/destruction.
/*class ScopedVoxelTextureRef
{
private:
	VoxelTextureID id;
	RendererSystem3D *rendererSystem;
public:
	ScopedVoxelTextureRef(VoxelTextureID id, RendererSystem3D &rendererSystem);
	~ScopedVoxelTextureRef();

	VoxelTextureID get() const;
};

class ScopedEntityTextureRef
{
private:
	EntityTextureID id;
	RendererSystem3D *rendererSystem;
public:
	ScopedEntityTextureRef(EntityTextureID id, RendererSystem3D &rendererSystem);
	~ScopedEntityTextureRef();

	EntityTextureID get() const;
};

class ScopedSkyTextureRef
{
private:
	SkyTextureID id;
	RendererSystem3D *rendererSystem;
public:
	ScopedSkyTextureRef(SkyTextureID id, RendererSystem3D &rendererSystem);
	~ScopedSkyTextureRef();

	SkyTextureID get() const;
};

class ScopedUiTextureRef
{
private:
	UiTextureID id;
	RendererSystem2D *rendererSystem;
public:
	ScopedUiTextureRef(UiTextureID id, RendererSystem2D &rendererSystem);
	~ScopedUiTextureRef();

	UiTextureID get() const;
};*/

#endif
