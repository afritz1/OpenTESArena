#include <algorithm>

#include "Chunk.h"

#include "components/debug/Debug.h"

Chunk::Chunk(int x, int y, int height)
{
	// Set all voxels to air and unused.
	this->voxels.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->voxels.fill(0);

	this->voxelDefs.fill(VoxelDefinition());
	this->activeVoxelDefs.fill(false);

	// Let the first voxel data (air) be usable immediately. All default voxel IDs can safely point to it.
	this->activeVoxelDefs.front() = true;

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

Chunk::VoxelID Chunk::get(int x, int y, int z) const
{
	return this->voxels.get(x, y, z);
}

const VoxelDefinition &Chunk::getVoxelDef(VoxelID id) const
{
	DebugAssert(id < this->voxelDefs.size());
	DebugAssert(this->activeVoxelDefs[id]);
	return this->voxelDefs[id];
}

int Chunk::debug_getVoxelDefCount() const
{
	return static_cast<int>(
		std::count(this->activeVoxelDefs.begin(), this->activeVoxelDefs.end(), true));
}

void Chunk::set(int x, int y, int z, VoxelID value)
{
	this->voxels.set(x, y, z, value);
}

Chunk::VoxelID Chunk::addVoxelDef(VoxelDefinition &&voxelDef)
{
	// Find a place to add the voxel data.
	const auto iter = std::find(this->activeVoxelDefs.begin(), this->activeVoxelDefs.end(), false);

	// If we ever hit this, we need more bits per voxel.
	DebugAssert(iter != this->activeVoxelDefs.end());

	const VoxelID id = static_cast<VoxelID>(std::distance(this->activeVoxelDefs.begin(), iter));
	this->voxelDefs[id] = std::move(voxelDef);
	this->activeVoxelDefs[id] = true;
	return id;
}

void Chunk::removeVoxelDef(VoxelID id)
{
	DebugAssert(id < this->voxelDefs.size());
	this->voxelDefs[id] = VoxelDefinition();
	this->activeVoxelDefs[id] = false;
}
