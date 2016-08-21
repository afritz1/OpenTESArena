#ifndef SPRITE_REFERENCE_H
#define SPRITE_REFERENCE_H

// A sprite reference is essentially the same as a voxel reference now in terms 
// of structural equivalence and behavior. I might decide to merge them together
// into a "RectReference" sometime, since the geometry is all rectangles instead 
// of triangles for voxels and pairs of triangles for sprites.

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
