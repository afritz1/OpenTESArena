#ifndef SPRITE_REFERENCE_H
#define SPRITE_REFERENCE_H

#include <cstdint>

// A sprite reference is essentially the same as a voxel reference now in terms 
// of structural equivalence and behavior. I might decide to merge them together
// into a "RectReference" sometime, since the geometry is all rectangles instead 
// of triangles for voxels and pairs of triangles for sprites.

class SpriteReference
{
private:
	int32_t offset, count;
public:
	SpriteReference(int32_t offset, int32_t count);
	~SpriteReference();

	int32_t getOffset() const;
	int32_t getRectangleCount() const;
};

#endif
