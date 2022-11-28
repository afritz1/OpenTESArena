#ifndef VOXEL_FACING_3D_H
#define VOXEL_FACING_3D_H

// Directions facing outward from a voxel. These can be matched with a face of the voxel for
// various calculations.

enum class VoxelFacing3D
{
	PositiveX,
	NegativeX,
	PositiveY,
	NegativeY,
	PositiveZ,
	NegativeZ
};

#endif
