#ifndef VOXEL_REFERENCE_H
#define VOXEL_REFERENCE_H

#include <cstdint>

// A voxel reference has an offset for how many rectangles to skip in the global
// geometry array for rendering, and the count tells how many rectangles to use at that
// offset. This class is intended for use with managing the kernel's more static geometry.

// If a voxel is empty, its voxel reference's count is zero, and its offset is essentially
// garbage. Theoretically, no two voxels should have their references share the same offset 
// and count because the geometry is stored relative to world space, not to a 3D offset in 
// the voxel grid.

// Example usage of a voxel reference in practice:
// - 3D-DDA algorithm: When a voxel is selected, use its offset member to jump into 
//   the 1D array of voxel rectangles in memory, and iterate from that point until 
//   the voxel's count is reached to see all rectangles in the voxel.

class VoxelReference
{
private:
	int32_t offset, count;
public:
	VoxelReference(int32_t offset, int32_t count);
	~VoxelReference();

	int32_t getOffset() const;
	int32_t getRectangleCount() const;
};

#endif
