#include "RenderCamera.h"

void RenderCamera::init(const ChunkInt2 &chunk, const Double3 &point, const Double3 &direction, double fovX,
	double fovY, double aspectRatio)
{
	this->chunk = chunk;
	this->point = point;
	this->forward = direction.normalized();
	this->right = this->forward.cross(Double3::UnitY).normalized();
	this->up = this->right.cross(this->forward).normalized();	
	this->fovX = fovX;
	this->fovY = fovY;
	this->aspectRatio = aspectRatio;
}
