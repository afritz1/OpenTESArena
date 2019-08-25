#include <algorithm>

#include "Chunk.h"

#include "components/debug/Debug.h"

Chunk::Chunk(int x, int y, int height)
{
	// Set all voxels to air and unused.
	this->voxels.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->voxels.fill(0);

	this->voxelData.fill(VoxelData());
	this->activeVoxelData.fill(false);

	// Let the first voxel data (air) be usable immediately. All default voxel IDs can safely point to it.
	this->activeVoxelData.front() = true;

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
	return this->voxels.getHeight();
}

constexpr int Chunk::getDepth() const
{
	return Chunk::DEPTH;
}

VoxelID Chunk::get(int x, int y, int z) const
{
	return *this->voxels.get(x, y, z);
}

const VoxelData &Chunk::getVoxelData(VoxelID id) const
{
	DebugAssert(id < this->voxelData.size());
	DebugAssert(this->activeVoxelData[id]);
	return this->voxelData[id];
}

int Chunk::debug_getVoxelDataCount() const
{
	return static_cast<int>(
		std::count(this->activeVoxelData.begin(), this->activeVoxelData.end(), true));
}

void Chunk::set(int x, int y, int z, VoxelID value)
{
	this->voxels.set(x, y, z, value);
}

VoxelID Chunk::addVoxelData(VoxelData &&voxelData)
{
	// Find a place to add the voxel data.
	const auto iter = std::find(this->activeVoxelData.begin(), this->activeVoxelData.end(), false);

	// If we ever hit this, we need more bits per voxel.
	DebugAssert(iter != this->activeVoxelData.end());

	const VoxelID id = static_cast<VoxelID>(std::distance(this->activeVoxelData.begin(), iter));
	this->voxelData[id] = std::move(voxelData);
	this->activeVoxelData[id] = true;
	return id;
}

void Chunk::removeVoxelData(VoxelID id)
{
	DebugAssert(id < this->voxelData.size());
	this->voxelData[id] = VoxelData();
	this->activeVoxelData[id] = false;
}
