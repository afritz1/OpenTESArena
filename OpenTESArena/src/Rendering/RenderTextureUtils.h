#ifndef RENDER_TEXTURE_UTILS_H
#define RENDER_TEXTURE_UTILS_H

// Common texture handles usable by all renderers in their public API when a user wants to
// allocate a new texture in the internal renderer format.

using VoxelTextureID = int; // Only for voxels, must be power-of-2 for mipmaps.
using SpriteTextureID = int; // Can be used for entities and sky objects, any dimensions.

#endif
