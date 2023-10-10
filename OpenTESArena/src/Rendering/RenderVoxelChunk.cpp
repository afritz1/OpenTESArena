#include <algorithm>

#include "Renderer.h"
#include "RendererSystem3D.h"
#include "RenderVoxelChunk.h"
#include "../World/ChunkUtils.h"

void RenderVoxelChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
	this->meshInstIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->meshInstIDs.fill(RenderVoxelChunk::AIR_MESH_INST_ID);
	this->meshInstMappings.emplace(VoxelChunk::AIR_MESH_DEF_ID, RenderVoxelChunk::AIR_MESH_INST_ID);

	// Add empty mesh instance for air.
	this->addMeshInst(RenderVoxelMeshInstance());
}

RenderVoxelMeshInstID RenderVoxelChunk::addMeshInst(RenderVoxelMeshInstance &&meshInst)
{
	const RenderVoxelMeshInstID id = static_cast<RenderVoxelMeshInstID>(this->meshInsts.size());
	this->meshInsts.emplace_back(std::move(meshInst));
	return id;
}

void RenderVoxelChunk::freeBuffers(Renderer &renderer)
{
	for (RenderVoxelMeshInstance &meshInst : this->meshInsts)
	{
		meshInst.freeBuffers(renderer);
	}

	for (const auto &pair : this->doorTransformBuffers)
	{
		renderer.freeUniformBuffer(pair.second);
	}
}

void RenderVoxelChunk::clear()
{
	Chunk::clear();
	this->meshInsts.clear();
	this->meshInstMappings.clear();
	this->meshInstIDs.clear();
	this->chasmWallIndexBufferIDsMap.clear();
	this->doorTransformBuffers.clear();
	this->staticDrawCalls.clear();
	this->doorDrawCalls.clear();
	this->chasmDrawCalls.clear();
	this->fadingDrawCalls.clear();
}
