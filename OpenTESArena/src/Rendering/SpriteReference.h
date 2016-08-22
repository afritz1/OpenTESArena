#ifndef SPRITE_REFERENCE_H
#define SPRITE_REFERENCE_H

// A sprite reference is essentially the same as a voxel reference now in terms 
// of structural equivalence and behavior. I might decide to merge them together
// into a "RectReference" sometime, since the geometry is all rectangles instead 
// of triangles for voxels and pairs of triangles for sprites.

// There's actually an issue to consider with having single-indirection for sprites. 
// If each sprite reference (per voxel) has an offset and count for the rectangles 
// array, then each sprite would likely have to have its geometry listed in the 
// array more than once, which is wasteful. However, I think double-indirection 
// would be too inefficient for the kernel.

// I suppose each sprite reference could just point to its own little chunk of
// independent data. This would increase memory usage and break the "one rectangle
// in the array per sprite" design, but would keep performance at least moderate,
// and would make better use of the cache.

// The "double-indirection" method would have each sprite reference point to an
// array of indices instead, and those indices would then point to unique instances
// of sprites in the rectangles array (a memory savings). I'm afraid this way would
// essentially throw the cache out the window (bad), and would saturate global memory
// with incoherent read requests (also bad).

// The single-indirection method sounds like a better idea: only one index used for
// accessing geometry at the expense of some duplicated geometry in memory. In
// reality, a sprite shouldn't need to be duplicated more than four or five times 
// (based on the voxels it touches), and it's currently only ~104 bytes.

class SpriteReference
{
private:
	int offset, count;
public:
	SpriteReference(int offset, int count);
	~SpriteReference();

	int getOffset() const;
	int getRectangleCount() const;
};

#endif
