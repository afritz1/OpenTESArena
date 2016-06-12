#ifndef VOXEL_REFERENCE_H
#define VOXEL_REFERENCE_H

// A voxel reference behaves in a similar way to a VoxelTemplate, except now there 
// are no real limits on the number of triangles allowed within (uint max?). Each 
// voxel reference can point to the same triangles if they are the same voxel, but 
// once a voxel starts to fade due to Passwall for example, it obtains its own list
// of triangles. If a voxel is empty, its voxel reference's triangle count is zero.

// Every voxel reference has triangles for voxels only. A sprite reference will take 
// care of the offset for sprite triangles (since a sprite is always two triangles).

// With the current offset and count model, each voxel will be 8 bytes.

// Example usage of a voxel reference in practice:
// - 3D-DDA algorithm: When a voxel is selected, use its offset member to jump into 
//   the 1D array of voxel triangles in memory, and iterate from that point until 
//   the voxel's count is reached to see all triangles in the voxel.

class VoxelReference
{
private:
	int offset, count;
public:
	VoxelReference(int offset, int count);
	~VoxelReference();

	int getOffset() const;
	int getTriangleCount() const;
};

#endif
