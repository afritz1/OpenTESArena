#include "RenderCamera.h"
#include "RendererUtils.h"

void RenderCamera::init(const ChunkInt2 &chunk, const Double3 &point, const Double3 &direction, Degrees fovY, double aspectRatio, double tallPixelRatio)
{
	this->chunk = chunk;
	this->chunkPoint = point;

	// @todo: eventually I think the chunk should be zeroed out and everything should always treat
	// the player's chunk as the origin chunk.
	this->worldPoint = VoxelUtils::chunkPointToWorldPoint(chunk, point);

	this->forward = direction.normalized();
	this->zoom = MathUtils::verticalFovToZoom(fovY);
	this->forwardScaled = this->forward * this->zoom;

	this->right = this->forward.cross(Double3::UnitY).normalized();
	this->aspectRatio = aspectRatio;
	this->rightScaled = this->right * this->aspectRatio;

	this->up = this->right.cross(this->forward).normalized();
	this->upScaled = this->up * tallPixelRatio;
	this->upScaledRecip = this->up / tallPixelRatio;

	this->viewMatrix = Matrix4d::view(this->worldPoint, this->forward, this->right, this->upScaled); // Adjust for tall pixels.
	this->projectionMatrix = Matrix4d::perspective(fovY, aspectRatio, RendererUtils::NEAR_PLANE, RendererUtils::FAR_PLANE);
	this->inverseViewMatrix = Matrix4d::inverseView(this->worldPoint, this->forward, this->right, this->upScaled);
	this->inverseProjectionMatrix = Matrix4d::inversePerspective(fovY, aspectRatio, RendererUtils::NEAR_PLANE, RendererUtils::FAR_PLANE);

	this->leftFrustumDir = (this->forwardScaled - this->rightScaled).normalized();
	this->rightFrustumDir = (this->forwardScaled + this->rightScaled).normalized();
	this->bottomFrustumDir = (this->forwardScaled - this->upScaledRecip).normalized();
	this->topFrustumDir = (this->forwardScaled + this->upScaledRecip).normalized();

	this->leftFrustumNormal = this->leftFrustumDir.cross(this->up).normalized();
	this->rightFrustumNormal = this->up.cross(this->rightFrustumDir).normalized();
	this->bottomFrustumNormal = this->right.cross(this->bottomFrustumDir).normalized();
	this->topFrustumNormal = this->topFrustumDir.cross(this->right).normalized();

	this->fovY = fovY;
	this->fovX = MathUtils::verticalFovToHorizontalFov(fovY, aspectRatio);
}

Double3 RenderCamera::screenToWorld(double xPercent, double yPercent) const
{
	const Double3 baseDir = this->forwardScaled - this->rightScaled + this->upScaledRecip;
	const Double3 adjustedDir = baseDir + (this->rightScaled * (2.0 * xPercent)) - (this->upScaledRecip * (2.0 * yPercent));
	return adjustedDir.normalized();
}
