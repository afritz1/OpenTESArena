#ifndef VOXEL_RENDER_INSTANCE_H
#define VOXEL_RENDER_INSTANCE_H

#include "../../World/VoxelUtils.h"

// In slight contrast to entity and sky object render instances, this is only spawned for "unique" voxels,
// not necessarily every voxel in the scene. Voxels are in an implicitly defined data structure, so they
// essentially are instances already just by being in a grid.

class VoxelRenderInstance
{
private:
	// @todo: unique state values (open door percent, fade percent, etc.)
	VoxelInt3 position;
	int defID;
public:

};

#endif
