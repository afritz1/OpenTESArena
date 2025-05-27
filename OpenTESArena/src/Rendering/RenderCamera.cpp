#include "RenderCamera.h"
#include "RendererUtils.h"
#include "../Math/MathUtils.h"

#include "components/debug/Debug.h"

void RenderCamera::init(const WorldDouble3 &worldPoint, Degrees yaw, Degrees pitch, Degrees fovY, double aspectRatio, double tallPixelRatio)
{
	this->worldPoint = worldPoint;

	const CoordDouble3 coord = VoxelUtils::worldPointToCoord(worldPoint);
	this->chunk = coord.chunk;
	this->chunkPoint = coord.point;

	MathUtils::populateCoordinateFrameFromAngles(yaw, pitch, &this->forward, &this->right, &this->up);
	this->zoom = MathUtils::verticalFovToZoom(fovY);
	this->forwardScaled = this->forward * this->zoom;

	this->aspectRatio = aspectRatio;
	this->rightScaled = this->right * aspectRatio;

	this->upScaled = this->up * tallPixelRatio;
	this->upScaledRecip = this->up / tallPixelRatio;

	this->viewMatrix = Matrix4d::view(this->worldPoint, this->forward, this->right, this->upScaled); // Adjust for tall pixels.
	this->projectionMatrix = Matrix4d::perspective(fovY, aspectRatio, RendererUtils::NEAR_PLANE, RendererUtils::FAR_PLANE);
	this->viewProjMatrix = this->projectionMatrix * this->viewMatrix;
	this->inverseViewMatrix = Matrix4d::inverse(this->viewMatrix);
	this->inverseProjectionMatrix = Matrix4d::inverse(this->projectionMatrix);

	this->leftFrustumDir = (this->forwardScaled - this->rightScaled).normalized();
	this->rightFrustumDir = (this->forwardScaled + this->rightScaled).normalized();
	this->bottomFrustumDir = (this->forwardScaled - this->upScaledRecip).normalized();
	this->topFrustumDir = (this->forwardScaled + this->upScaledRecip).normalized();

	this->leftFrustumNormal = this->leftFrustumDir.cross(this->up).normalized();
	this->rightFrustumNormal = this->up.cross(this->rightFrustumDir).normalized();
	this->bottomFrustumNormal = this->right.cross(this->bottomFrustumDir).normalized();
	this->topFrustumNormal = this->topFrustumDir.cross(this->right).normalized();

	this->horizonDir = Double3(this->forward.x, 0.0, this->forward.z).normalized();
	this->horizonNormal = Double3::UnitY;

	// @todo: this doesn't support roll. will need something like a vector projection later.
	this->horizonWorldPoint = this->worldPoint + this->horizonDir;
	this->horizonCameraPoint = RendererUtils::worldSpaceToCameraSpace(Double4(this->horizonWorldPoint, 1.0), this->viewMatrix);
	this->horizonClipPoint = RendererUtils::cameraSpaceToClipSpace(this->horizonCameraPoint, this->projectionMatrix);
	this->horizonNdcPoint = RendererUtils::clipSpaceToNDC(this->horizonClipPoint);

	this->fovY = fovY;
	this->fovX = MathUtils::verticalFovToHorizontalFov(fovY, aspectRatio);
}

Double3 RenderCamera::screenToWorld(double xPercent, double yPercent) const
{
	const Double3 baseDir = this->forwardScaled - this->rightScaled + this->upScaledRecip;
	const Double3 adjustedDir = baseDir + (this->rightScaled * (2.0 * xPercent)) - (this->upScaledRecip * (2.0 * yPercent));
	return adjustedDir.normalized();
}
