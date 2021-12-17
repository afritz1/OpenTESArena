#ifndef RENDER_CAMERA_H
#define RENDER_CAMERA_H

#include "../Math/Vector3.h"
#include "../World/VoxelUtils.h"

// Common render camera usable by all renderers.

struct RenderCamera
{
	ChunkInt2 chunk;
	VoxelDouble3 point, direction;
	double fovX, fovY;

	void init(const ChunkInt2 &chunk, const VoxelDouble3 &point, const VoxelDouble3 &direction, double fovX, double fovY);
};

#endif
