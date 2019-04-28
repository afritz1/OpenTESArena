#include <algorithm>
#include <cassert>

#include "Chunk.h"

Chunk::Chunk(int x, int y, int height)
{
	// Set all voxels to air and unused.
	const int voxelCount = Chunk::WIDTH * height * Chunk::DEPTH;
	this->voxels = std::make_unique<VoxelID[]>(voxelCount);
	std::fill(this->voxels.get(), this->voxels.get() + voxelCount, 0);

	this->voxelData.fill(VoxelData());
	this->activeVoxelData.fill(false);

	// Let the first voxel data (air) be usable immediately. All default voxel IDs can safely point to it.
	this->activeVoxelData.front() = true;

	this->height = height;
	this->x = x;
	this->y = y;
}

int Chunk::getX() const
{
	return this->x;
}

int Chunk::getY() const
{
	return this->y;
}

constexpr int Chunk::getWidth() const
{
	return Chunk::WIDTH;
}

int Chunk::getHeight() const
{
	return this->height;
}

constexpr int Chunk::getDepth() const
{
	return Chunk::DEPTH;
}

bool Chunk::coordIsValid(int x, int y, int z) const
{
	return (x >= 0) && (x < this->getWidth()) &&
		(y >= 0) && (y < this->getHeight()) &&
		(z >= 0) && (z < this->getDepth());
}

int Chunk::getIndex(int x, int y, int z) const
{
	assert(this->coordIsValid(x, y, z));
	return x + (y * this->getWidth()) + (z * this->getWidth() * this->getHeight());
}

VoxelID Chunk::get(int x, int y, int z) const
{
	const int index = this->getIndex(x, y, z);
	return this->voxels[index];
}

const VoxelData &Chunk::getVoxelData(VoxelID id) const
{
	assert(id < this->voxelData.size());
	assert(this->activeVoxelData[id]);
	return this->voxelData[id];
}

int Chunk::debug_getVoxelDataCount() const
{
	return static_cast<int>(
		std::count(this->activeVoxelData.begin(), this->activeVoxelData.end(), true));
}

void Chunk::set(int x, int y, int z, VoxelID value)
{
	const int index = this->getIndex(x, y, z);
	this->voxels[index] = value;
}

VoxelID Chunk::addVoxelData(VoxelData &&voxelData)
{
	// Find a place to add the voxel data.
	const auto iter = std::find(this->activeVoxelData.begin(), this->activeVoxelData.end(), false);

	// If we ever hit this, we need more bits per voxel.
	assert(iter != this->activeVoxelData.end());

	const VoxelID id = static_cast<VoxelID>(std::distance(this->activeVoxelData.begin(), iter));
	this->voxelData[id] = std::move(voxelData);
	this->activeVoxelData[id] = true;
	return id;
}

void Chunk::removeVoxelData(VoxelID id)
{
	assert(id < this->voxelData.size());
	this->voxelData[id] = VoxelData();
	this->activeVoxelData[id] = false;
}
