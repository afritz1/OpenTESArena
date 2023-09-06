#include <algorithm>

#include "RenderChunk.h"
#include "Renderer.h"
#include "RendererSystem3D.h"
#include "../World/ChunkUtils.h"

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

void RenderChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
	this->meshDefIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->meshDefIDs.fill(RenderChunk::AIR_MESH_DEF_ID);
	this->meshDefMappings.emplace(VoxelChunk::AIR_MESH_DEF_ID, RenderChunk::AIR_MESH_DEF_ID);
	this->voxelLightIdLists.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);

	// Add empty mesh instance for air.
	this->addMeshDefinition(RenderVoxelMeshDefinition());
}

RenderVoxelMeshDefID RenderChunk::addMeshDefinition(RenderVoxelMeshDefinition &&meshDef)
{
	const RenderVoxelMeshDefID id = static_cast<RenderVoxelMeshDefID>(this->meshDefs.size());
	this->meshDefs.emplace_back(std::move(meshDef));
	return id;
}

void RenderChunk::addDirtyLightPosition(const VoxelInt3 &position)
{
	const auto iter = std::find(this->dirtyLightPositions.begin(), this->dirtyLightPositions.end(), position);
	if (iter == this->dirtyLightPositions.end())
	{
		this->dirtyLightPositions.emplace_back(position);
	}
}

void RenderChunk::freeBuffers(Renderer &renderer)
{
	for (RenderVoxelMeshDefinition &meshDef : this->meshDefs)
	{
		meshDef.freeBuffers(renderer);
	}

	for (const auto &pair : this->doorTransformBuffers)
	{
		renderer.freeUniformBuffer(pair.second);
	}
}

void RenderChunk::clear()
{
	Chunk::clear();
	this->meshDefs.clear();
	this->meshDefMappings.clear();
	this->meshDefIDs.clear();
	this->chasmWallIndexBufferIDsMap.clear();
	this->doorTransformBuffers.clear();
	this->voxelLightIdLists.clear();
	this->dirtyLightPositions.clear();
	this->staticDrawCalls.clear();
	this->doorDrawCalls.clear();
	this->chasmDrawCalls.clear();
	this->fadingDrawCalls.clear();
	this->entityDrawCalls.clear();
}
