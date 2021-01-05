#ifndef RENDER_TEXTURE_UTILS_H
#define RENDER_TEXTURE_UTILS_H

// Common texture handles allocated by a renderer for a user when they want a new texture in the
// internal renderer format.

using VoxelTextureID = int; // Only for voxels, must be power-of-2 for mipmaps.
using EntityTextureID = int; // One per frame of entity animations, any dimensions.
using SkyTextureID = int; // Similar to entity textures but for mountains/clouds/stars/etc.
using UiTextureID = int; // Used with all UI textures.

#endif
