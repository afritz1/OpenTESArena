#include <algorithm>

#include "RenderLightChunk.h"

RenderVoxelLightIdList::RenderVoxelLightIdList()
{
	this->clear();
}

BufferView<RenderLightID> RenderVoxelLightIdList::getLightIDs()
{
	return BufferView<RenderLightID>(this->lightIDs, this->lightCount);
}

BufferView<const RenderLightID> RenderVoxelLightIdList::getLightIDs() const
{
	return BufferView<const RenderLightID>(this->lightIDs, this->lightCount);
}

void RenderVoxelLightIdList::tryAddLight(RenderLightID id)
{
	if (this->lightCount >= static_cast<int>(std::size(this->lightIDs)))
	{
		return;
	}

	this->lightIDs[this->lightCount] = id;
	this->lightCount++;
}

void RenderVoxelLightIdList::clear()
{
	std::fill(std::begin(this->lightIDs), std::end(this->lightIDs), -1);
	this->lightCount = 0;
}

void RenderLightChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
	this->voxelLightIdLists.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
}

void RenderLightChunk::addDirtyLightPosition(const VoxelInt3 &position)
{
	if (!this->isValidVoxel(position.x, position.y, position.z))
	{
		DebugLogWarning("Invalid dirty light position (" + position.toString() + ") in chunk (" + this->getPosition().toString() + ").");
		return;
	}

	const auto iter = std::find(this->dirtyLightPositions.begin(), this->dirtyLightPositions.end(), position);
	if (iter == this->dirtyLightPositions.end())
	{
		this->dirtyLightPositions.emplace_back(position);
	}
}

void RenderLightChunk::clear()
{
	Chunk::clear();
	this->voxelLightIdLists.clear();
	this->dirtyLightPositions.clear();
}
