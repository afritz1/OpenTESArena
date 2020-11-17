#ifndef RENDER_CAMERA_H
#define RENDER_CAMERA_H

#include "../Math/Vector3.h"
#include "../World/VoxelUtils.h"

// Common render camera usable by all renderers.

class RenderCamera
{
private:
	ChunkInt2 chunk;
	VoxelDouble3 voxel, direction;
	double fovX, fovY;
public:
	// @todo
};

#endif
