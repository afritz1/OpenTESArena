#include <algorithm>

#include "Renderer.h"
#include "RendererSystem3D.h"
#include "RenderVoxelChunk.h"
#include "../World/ChunkUtils.h"

void RenderVoxelChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
	this->meshDefIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->meshDefIDs.fill(RenderVoxelChunk::AIR_MESH_DEF_ID);
	this->meshDefMappings.emplace(VoxelChunk::AIR_MESH_DEF_ID, RenderVoxelChunk::AIR_MESH_DEF_ID);
	this->voxelLightIdLists.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);

	// Add empty mesh instance for air.
	this->addMeshDefinition(RenderVoxelMeshDefinition());
}

RenderVoxelMeshDefID RenderVoxelChunk::addMeshDefinition(RenderVoxelMeshDefinition &&meshDef)
{
	const RenderVoxelMeshDefID id = static_cast<RenderVoxelMeshDefID>(this->meshDefs.size());
	this->meshDefs.emplace_back(std::move(meshDef));
	return id;
}

void RenderVoxelChunk::addDirtyLightPosition(const VoxelInt3 &position)
{
	const auto iter = std::find(this->dirtyLightPositions.begin(), this->dirtyLightPositions.end(), position);
	if (iter == this->dirtyLightPositions.end())
	{
		this->dirtyLightPositions.emplace_back(position);
	}
}

void RenderVoxelChunk::freeBuffers(Renderer &renderer)
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

void RenderVoxelChunk::clear()
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
}
