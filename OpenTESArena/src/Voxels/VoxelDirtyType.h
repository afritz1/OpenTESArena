#ifndef VOXEL_DIRTY_TYPE_H
#define VOXEL_DIRTY_TYPE_H

#include <cstdint>

enum class VoxelDirtyType : uint8_t
{
	MeshDefinition = (1 << 0),
	DoorAnimation = (1 << 1),
	DoorVisibility = (1 << 2),
	ChasmWall = (1 << 3),
	FadeAnimation = (1 << 4)
};

#endif
