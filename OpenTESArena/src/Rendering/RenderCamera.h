#ifndef RENDER_CAMERA_H
#define RENDER_CAMERA_H

#include "../Math/Vector3.h"
#include "../World/VoxelUtils.h"

// Common render camera usable by all renderers.

struct RenderCamera
{
	ChunkInt2 chunk;
	Double3 point; // 3D position relative to chunk origin.
	Double3 forward, right, up;
	double fovX, fovY, aspectRatio;

	void init(const ChunkInt2 &chunk, const Double3 &point, const Double3 &direction, double fovX,
		double fovY, double aspectRatio);
};

#endif
