#include "ChunkRenderDefinition.h"

#include "components/debug/Debug.h"

void ChunkRenderDefinition::init(SNInt width, int height, WEInt depth, const ChunkInt2 &coord)
{
	this->voxelRenderDefIDs.init(width, height, depth);
	this->voxelRenderDefIDs.fill(ChunkRenderDefinition::NO_VOXEL_ID);
	this->coord = coord;
}

const ChunkInt2 &ChunkRenderDefinition::getCoord() const
{
	return this->coord;
}

const VoxelRenderDefinition &ChunkRenderDefinition::getVoxelRenderDef(VoxelRenderDefID id) const
{
	DebugAssertIndex(this->voxelRenderDefs, id);
	return this->voxelRenderDefs[id];
}

SNInt ChunkRenderDefinition::getWidth() const
{
	return this->voxelRenderDefIDs.getWidth();
}

int ChunkRenderDefinition::getHeight() const
{
	return this->voxelRenderDefIDs.getHeight();
}

WEInt ChunkRenderDefinition::getDepth() const
{
	return this->voxelRenderDefIDs.getDepth();
}

VoxelRenderDefID ChunkRenderDefinition::getVoxelRenderDefID(SNInt x, int y, WEInt z) const
{
	return this->voxelRenderDefIDs.get(x, y, z);
}

VoxelRenderDefID ChunkRenderDefinition::addVoxelRenderDef(VoxelRenderDefinition &&def)
{
	const VoxelRenderDefID id = static_cast<VoxelRenderDefID>(this->voxelRenderDefs.size());
	this->voxelRenderDefs.emplace_back(std::move(def));
	return id;
}

void ChunkRenderDefinition::clear()
{
	this->voxelRenderDefs.clear();
	this->voxelRenderDefIDs.fill(ChunkRenderDefinition::NO_VOXEL_ID);
}
