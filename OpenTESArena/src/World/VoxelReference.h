#ifndef VOXEL_REFERENCE_H
#define VOXEL_REFERENCE_H

// A voxel reference behaves in a similar way to a VoxelTemplate except that the 
// triangles it points to are unique to the voxel, and there are no limits on the 
// number of triangles allowed within.

// Every voxel reference can have triangles from walls and sprites.

// No two voxel references should point to the same triangles. Though this would
// be a nice memory savings, it makes managing them more complicated when a voxel
// changes. We're not modeling a multi-million polygon shape; it's just a few
// hundred cubes and sprites, so memory should not be the primary concern.

// Example usage of a voxel reference in practice:
// 1) 3D-DDA algorithm.
// -> When a voxel is selected, use its offset member to jump into the giant 1D 
//    array of triangles in memory, and iterate from that point until the voxel's 
//    count is reached to see all triangles in the voxel. No need to check sprites 
//    separately because they are just triangles, too.
// 2) Moving sprites.
// -> For every frame, when a sprite moves, check the voxels they were in before,
//    and the voxels they are in now. For any voxels that no longer contain them,
//    refresh their voxel reference and the voxels they reference. A sprite reference 
//    count should be used to see which sprites are in which voxels, because triangles 
//    don't have enough information by themselves to know which to remove.

class VoxelReference
{
private:
	int offset, count;
public:
	VoxelReference(int offset, int count);
	~VoxelReference();

	const int &getOffset() const;
	const int &getTriangleCount() const;
};

#endif
