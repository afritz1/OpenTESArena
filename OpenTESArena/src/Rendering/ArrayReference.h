#ifndef ARRAY_REFERENCE_H
#define ARRAY_REFERENCE_H

// An "array reference" just points into a buffer at some offset and designates 
// how many elements are usable at that offset. Intended for OpenCL memory.

// All array references are 8 bytes for consistency.

// Texture references are separate because they're 2D.

// These reference structs help the host maintain device memory.
// - Voxel references point to arrays of rectangles.
// - Sprite references point to arrays of rectangles.
// - Light references point to arrays of lights.
// - Owner references point to arrays of indices.
// - Texture references point to 2D arrays of float4's.

struct ArrayReference
{
	// Offset is in units of elements in the referred array.
	int offset, count;

	ArrayReference(int offset, int count);
};

typedef ArrayReference VoxelReference;
typedef ArrayReference SpriteReference;
typedef ArrayReference LightReference;
typedef ArrayReference OwnerReference;

struct TextureReference
{
	// Offset is the number of float4's to skip.
	int offset;
	short width, height;

	TextureReference(int offset, short width, short height);
};

#endif
