#include <algorithm>

#include "RenderChunk.h"
#include "RendererSystem3D.h"
#include "../World/ChunkUtils.h"

void RenderChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
	this->meshDefs.clear();
	this->meshDefMappings.clear();
	this->meshDefIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->chasmWallIndexBufferIDs.clear();
	this->staticDrawCalls.clear();
	this->animatingDrawCalls.clear();

	// Add empty mesh instance for air.
	this->addMeshDefinition(RenderVoxelMeshDefinition());
}

void RenderChunk::update()
{
	DebugNotImplemented();
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
