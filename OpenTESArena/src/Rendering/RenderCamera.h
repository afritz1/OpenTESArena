#ifndef RENDER_CAMERA_H
#define RENDER_CAMERA_H

#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector3.h"
#include "../Voxels/VoxelUtils.h"

// Common render camera usable by all renderers.
struct RenderCamera
{
	ChunkInt2 chunk;
	Double3 chunkPoint; // 3D position relative to chunk origin.
	Double3 worldPoint; // 3D position relative to world origin.
	Double3 forward, right, up;
	Double3 forwardScaled; // Scaled by zoom.
	Double3 rightScaled; // Scaled by aspect ratio.
	Double3 upScaled; // Scaled by tall pixel ratio.
	Double3 upScaledRecip; // Scaled by 1.0 / tall pixel ratio.

	Matrix4d viewMatrix;
	Matrix4d projectionMatrix;
	Matrix4d viewProjMatrix;
	Matrix4d inverseViewMatrix;
	Matrix4d inverseProjectionMatrix;

	// Frustum directions pointing away from the camera eye.
	Double3 leftFrustumDir, rightFrustumDir, bottomFrustumDir, topFrustumDir;

	// Frustum plane normals pointing towards the inside of the frustum volume.
	Double3 leftFrustumNormal, rightFrustumNormal, bottomFrustumNormal, topFrustumNormal;

	// Horizon values for mirroring effects.
	Double3 horizonDir;
	Double3 horizonNormal; // Global up.
	Double3 horizonWorldPoint;
	Double4 horizonCameraPoint;
	Double4 horizonClipPoint;
	Double3 horizonNdcPoint;

	Degrees fovX, fovY;
	double zoom; // Function of vertical FOV (90 degrees = 1 zoom).
	double aspectRatio;

	void init(const ChunkInt2 &chunk, const Double3 &point, const Double3 &direction, Degrees fovY,
		double aspectRatio, double tallPixelRatio);

	// Generates a 3D ray from an XY position on-screen.
	Double3 screenToWorld(double xPercent, double yPercent) const;
};

#endif
