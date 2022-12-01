#include <algorithm>

#include "RenderChunk.h"
#include "RendererSystem3D.h"
#include "../World/ChunkUtils.h"

void RenderChunk::init(const ChunkInt2 &position, int height)
{
	this->meshInsts.clear();
	this->meshInstMappings.clear();
	this->meshInstIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->chasmWallIndexBufferIDs.clear();
	this->staticDrawCalls.clear();
	this->animatingDrawCalls.clear();
	this->position = position;

	// Add empty mesh instance for air.
	this->addMeshInstance(RenderChunkVoxelMeshInstance());
}

void RenderChunk::update()
{
	DebugNotImplemented();
}

RenderChunkVoxelMeshInstanceID RenderChunk::addMeshInstance(RenderChunkVoxelMeshInstance &&meshInst)
{
	const RenderChunkVoxelMeshInstanceID id = static_cast<RenderChunkVoxelMeshInstanceID>(this->meshInsts.size());
	this->meshInsts.emplace_back(std::move(meshInst));
	return id;
}

void RenderChunk::freeBuffers(RendererSystem3D &renderer)
{
	for (RenderChunkVoxelMeshInstance &meshInst : this->meshInsts)
	{
		meshInst.freeBuffers(renderer);
	}
}
