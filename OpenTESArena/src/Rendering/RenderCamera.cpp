#include "RenderCamera.h"
#include "RendererUtils.h"
#include "../Math/MathUtils.h"

#include "components/debug/Debug.h"

RenderCamera::RenderCamera()
{
	this->yaw = 0.0;
	this->pitch = 0.0;
	this->fovX = 0.0;
	this->fovY = 0.0;
	this->zoom = 0.0;
	this->aspectRatio = 0.0;
	this->tallPixelRatio = 0.0;
}

void RenderCamera::init(const WorldDouble3 &worldPoint, Degrees yaw, Degrees pitch, Degrees fovY, double aspectRatio, double tallPixelRatio)
{
	this->worldPoint = worldPoint;
	this->chunk = VoxelUtils::worldPointToChunk(worldPoint);
	this->floatingOriginPoint = VoxelUtils::chunkPointToWorldPoint(this->chunk, VoxelDouble3::Zero);
	this->floatingWorldPoint = worldPoint - this->floatingOriginPoint;

	this->yaw = yaw;
	this->pitch = pitch;
	MathUtils::populateCoordinateFrameFromAngles(yaw, pitch, &this->forward, &this->right, &this->up);
	this->zoom = MathUtils::verticalFovToZoom(fovY);
	this->forwardScaled = this->forward * this->zoom;

	this->aspectRatio = aspectRatio;
	this->rightScaled = this->right * aspectRatio;

	this->tallPixelRatio = tallPixelRatio;
	this->upScaled = this->up * tallPixelRatio;
	this->upScaledRecip = this->up / tallPixelRatio;

	this->viewMatrix = Matrix4d::view(this->floatingWorldPoint, this->forward, this->right, this->upScaled); // Adjust for tall pixels.
	this->projectionMatrix = Matrix4d::perspective(fovY, aspectRatio, RendererUtils::NEAR_PLANE, RendererUtils::FAR_PLANE);
	this->viewProjMatrix = this->projectionMatrix * this->viewMatrix;
	this->inverseViewMatrix = Matrix4d::inverse(this->viewMatrix);
	this->inverseProjectionMatrix = Matrix4d::inverse(this->projectionMatrix);

	this->createFrustumVectors(0.0, 1.0, 0.0, 1.0, &this->leftFrustumDir, &this->rightFrustumDir, &this->bottomFrustumDir, &this->topFrustumDir,
		&this->leftFrustumNormal, &this->rightFrustumNormal, &this->bottomFrustumNormal, &this->topFrustumNormal);

	this->horizonDir = Double3(this->forward.x, 0.0, this->forward.z).normalized();
	this->horizonNormal = Double3::UnitY;

	// @todo: this doesn't support roll. will need something like a vector projection later.
	this->horizonWorldPoint = this->floatingWorldPoint + this->horizonDir;
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

void RenderCamera::createFrustumVectors(double startXPercent, double endXPercent, double startYPercent, double endYPercent,
	Double3 *outFrustumDirLeft, Double3 *outFrustumDirRight, Double3 *outFrustumDirBottom, Double3 *outFrustumDirTop,
	Double3 *outFrustumNormalLeft, Double3 *outFrustumNormalRight, Double3 *outFrustumNormalBottom, Double3 *outFrustumNormalTop) const
{
	DebugAssert(endXPercent >= startXPercent);
	DebugAssert(endYPercent >= startYPercent);

	const Double3 baseVectorX = this->forwardScaled - this->rightScaled;
	const Double3 baseVectorY = this->forwardScaled + this->upScaledRecip;
	const Double3 frustumBeginRightComponent = this->rightScaled * (2.0 * startXPercent);
	const Double3 frustumEndRightComponent = this->rightScaled * (2.0 * endXPercent);
	const Double3 frustumBeginUpComponent = this->upScaledRecip * (2.0 * startYPercent);
	const Double3 frustumEndUpComponent = this->upScaledRecip * (2.0 * endYPercent);

	*outFrustumDirLeft = (baseVectorX + frustumBeginRightComponent).normalized();
	*outFrustumDirRight = (baseVectorX + frustumEndRightComponent).normalized();
	*outFrustumDirBottom = (baseVectorY - frustumEndUpComponent).normalized();
	*outFrustumDirTop = (baseVectorY - frustumBeginUpComponent).normalized();

	*outFrustumNormalLeft = outFrustumDirLeft->cross(this->up).normalized();
	*outFrustumNormalRight = this->up.cross(*outFrustumDirRight).normalized();
	*outFrustumNormalBottom = this->right.cross(*outFrustumDirBottom).normalized();
	*outFrustumNormalTop = outFrustumDirTop->cross(this->right).normalized();
}
