#ifndef VOXEL_FACING_H
#define VOXEL_FACING_H

// Defines which axis a voxel's normal is facing towards on the outside (i.e., away from the
// center of the voxel). Does not apply to special-case voxels like diagonals.

enum class VoxelFacing
{
	PositiveX,
	NegativeX,
	PositiveY,
	NegativeY,
	PositiveZ,
	NegativeZ
};

#endif
