#include "RenderCamera.h"

void RenderCamera::init(const ChunkInt2 &chunk, const VoxelDouble3 &point, const VoxelDouble3 &direction, double fovX, double fovY)
{
	this->chunk = chunk;
	this->point = point;
	this->direction = direction;
	this->fovX = fovX;
	this->fovY = fovY;
}
