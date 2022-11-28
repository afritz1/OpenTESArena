#ifndef RENDER_CAMERA_H
#define RENDER_CAMERA_H

#include "../Math/MathUtils.h"
#include "../Math/Vector3.h"
#include "../Voxels/VoxelUtils.h"

// Common render camera usable by all renderers.

struct RenderCamera
{
	ChunkInt2 chunk;
	Double3 point; // 3D position relative to chunk origin.
	Double3 forward, right, up;
	Double3 forwardScaled; // Scaled by zoom.
	Double3 rightScaled; // Scaled by aspect ratio.
	Double3 upScaled; // Scaled by tall pixel ratio.

	// Frustum directions pointing away from the camera eye.
	Double3 leftFrustumDir, rightFrustumDir, bottomFrustumDir, topFrustumDir;

	// Frustum plane normals pointing towards the inside of the frustum volume.
	Double3 leftFrustumNormal, rightFrustumNormal, bottomFrustumNormal, topFrustumNormal;

	Degrees fovX, fovY;
	double zoom; // Function of vertical FOV (90 degrees = 1 zoom).
	double aspectRatio;
	double tallPixelRatio;

	void init(const ChunkInt2 &chunk, const Double3 &point, const Double3 &direction, Degrees fovX,
		Degrees fovY, double aspectRatio, double tallPixelRatio);
};

#endif
