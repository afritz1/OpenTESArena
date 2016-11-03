#ifndef VOXEL_BUILDER_H
#define VOXEL_BUILDER_H

#include <vector>

// Static class for generating voxel geometry.

// For block faces that only display a portion of a texture, that portion 
// of the texture should be a new texture in device memory so the kernel
// can maintain its ability to infer texture coordinates.

class Rect3D;

class VoxelBuilder
{
private:
	VoxelBuilder() = delete;
	VoxelBuilder(const VoxelBuilder&) = delete;
	~VoxelBuilder() = delete;
public:
	// Creates a variable-height block at the given voxel coordinates. The y1 and y2 
	// values determine where the block starts and stops relative to the voxel's Y,
	// and y2 must be greater than y1.
	static std::vector<Rect3D> makeSizedBlock(int x, int y, int z, float y1, float y2);

	// Creates a 1x1x1 block at the given coordinates.
	static std::vector<Rect3D> makeBlock(int x, int y, int z);

	// Creates a ceiling at the given voxel coordinates. Useful for the top of a 
	// ground block, since none of the geometry under it would be visible.
	static Rect3D makeCeiling(int x, int y, int z);

	// Creates a floor at the given voxel coordinates. Useful for the bottom of a 
	// ceiling block, since none of the geometry above it would be visible.
	static Rect3D makeFloor(int x, int y, int z);

	// Creates a block with an empty top and bottom. Useful for hedges.
	static std::vector<Rect3D> makeHollowY(int x, int y, int z);

	// Creates a block with an empty top, bottom, front, and back. Useful for arches
	// facing north/south (along X axis).
	static std::vector<Rect3D> makeHollowYZ(int x, int y, int z);

	// Creates a block with an empty top, bottom, left, and right. Useful for arches
	// facing east/west (along Z axis).
	static std::vector<Rect3D> makeHollowXY(int x, int y, int z);
};

#endif
