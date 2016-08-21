#include "Chunk.h"

Chunk::Chunk(const Voxel &fillVoxel)
{
	// Set all of this chunk's voxels to the given voxel.
	for (auto &voxel : this->voxels)
	{
		voxel = fillVoxel;
	}
}

Chunk::Chunk()
{

}

Chunk::~Chunk()
{

}

const Voxel &Chunk::get(int32_t x, int32_t y, int32_t z)
{
	return this->voxels.at(x + (y * Chunk::Width) +
		(z * Chunk::Width * Chunk::Height));
}

void Chunk::set(int32_t x, int32_t y, int32_t z, const Voxel &voxel)
{
	this->voxels.at(x + (y * Chunk::Width) +
		(z * Chunk::Width * Chunk::Height)) = voxel;
}
