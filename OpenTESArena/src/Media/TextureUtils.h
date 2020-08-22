#ifndef TEXTURE_UTILS_H
#define TEXTURE_UTILS_H

// Various texture handles for use with texture manager.
using PaletteID = int; // 256 color 32-bit software surface
using ImageID = int; // 8-bit software surface
using SurfaceID = int; // 32-bit software surface
using TextureID = int; // 32-bit hardware surface

// Texture instance handles, same as texture manager but for generated textures not loaded
// from a file.
using ImageInstanceID = int; // 8-bit software surface
using SurfaceInstanceID = int; // 32-bit software surface
using TextureInstanceID = int; // 32-bit hardware surface

#endif
