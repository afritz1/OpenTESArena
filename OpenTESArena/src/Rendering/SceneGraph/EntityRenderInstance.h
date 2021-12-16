#ifndef ENTITY_RENDER_INSTANCE_H
#define ENTITY_RENDER_INSTANCE_H

#include "../../World/Coord.h"

struct EntityRenderInstance
{
	VoxelDouble3 position; // Base of entity relative to camera chunk's origin.
	double width, height; // Dimensions of world space geometry.
	int defID;
};

#endif
