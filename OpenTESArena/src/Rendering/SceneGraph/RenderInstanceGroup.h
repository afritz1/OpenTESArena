#ifndef RENDER_INSTANCE_GROUP_H
#define RENDER_INSTANCE_GROUP_H

#include "EntityRenderInstance.h"
#include "SkyObjectRenderInstance.h"
#include "VoxelRenderInstance.h"

// All unique instances of voxels/entities/sky-objects in the game world that have positions,
// shader variables, etc. for their current state.

class RenderInstanceGroup
{
private:
	// @todo: all voxel/entity/sky-object render instances, with whatever references into
	// render definition group entries needed.
public:

};

#endif
