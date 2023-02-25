#include <algorithm>

#include "RenderChunk.h"
#include "RendererSystem3D.h"
#include "../World/ChunkUtils.h"

void RenderChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
	this->meshDefIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->meshDefIDs.fill(RenderChunk::AIR_MESH_DEF_ID);
	this->meshDefMappings.emplace(VoxelChunk::AIR_MESH_DEF_ID, RenderChunk::AIR_MESH_DEF_ID);

	// Add empty mesh instance for air.
	this->addMeshDefinition(RenderVoxelMeshDefinition());
}

RenderVoxelMeshDefID RenderChunk::addMeshDefinition(RenderVoxelMeshDefinition &&meshDef)
{
	const RenderVoxelMeshDefID id = static_cast<RenderVoxelMeshDefID>(this->meshDefs.size());
	this->meshDefs.emplace_back(std::move(meshDef));
	return id;
}

void RenderChunk::freeBuffers(Renderer &renderer)
{
	for (RenderVoxelMeshDefinition &meshDef : this->meshDefs)
	{
		meshDef.freeBuffers(renderer);
	}
}

void RenderChunk::clear()
{
	Chunk::clear();
	this->meshDefs.clear();
	this->meshDefMappings.clear();
	this->meshDefIDs.clear();
	this->chasmWallIndexBufferIDs.clear();
	this->staticDrawCalls.clear();
	this->doorDrawCalls.clear();
	this->chasmDrawCalls.clear();
	this->fadingDrawCalls.clear();
	this->entityDrawCalls.clear();
}
