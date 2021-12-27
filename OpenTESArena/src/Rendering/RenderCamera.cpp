#include "RenderCamera.h"

void RenderCamera::init(const ChunkInt2 &chunk, const Double3 &point, const Double3 &direction, Degrees fovX,
	Degrees fovY, double aspectRatio)
{
	this->chunk = chunk;
	this->point = point;
	
	this->forward = direction.normalized();
	this->zoom = MathUtils::verticalFovToZoom(fovY);
	this->forwardScaled = this->forward * this->zoom;

	this->right = this->forward.cross(Double3::UnitY).normalized();
	this->aspectRatio = aspectRatio;
	this->rightScaled = this->right * this->aspectRatio;

	this->up = this->right.cross(this->forward).normalized();	

	this->fovX = fovX;
	this->fovY = fovY;
}
