#include "ChunkRenderInstance.h"

#include "components/debug/Debug.h"

void ChunkRenderInstance::init(int defID, const ChunkInt2 &coord)
{
	this->defID = defID;
	this->coord = coord;
}

int ChunkRenderInstance::getDefID() const
{
	return this->defID;
}

const ChunkInt2 &ChunkRenderInstance::getCoord() const
{
	return this->coord;
}

int ChunkRenderInstance::getVoxelRenderInstanceCount() const
{
	return static_cast<int>(this->voxelRenderInsts.size());
}

const VoxelRenderInstance &ChunkRenderInstance::getVoxelRenderInstance(int index) const
{
	DebugAssertIndex(this->voxelRenderInsts, index);
	return this->voxelRenderInsts[index];
}

void ChunkRenderInstance::addVoxelRenderInstance(VoxelRenderInstance &&inst)
{
	this->voxelRenderInsts.emplace_back(std::move(inst));
}

void ChunkRenderInstance::clear()
{
	this->voxelRenderInsts.clear();
}
