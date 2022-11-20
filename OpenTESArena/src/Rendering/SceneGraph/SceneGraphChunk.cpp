#include <algorithm>

#include "SceneGraphChunk.h"
#include "../RendererSystem3D.h"
#include "../../World/ChunkUtils.h"

void SceneGraphChunk::init(const ChunkInt2 &position, int height)
{
	this->meshInsts.clear();
	this->meshInstMappings.clear();
	this->meshInstIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->chasmWallIndexBufferIDs.clear();
	this->staticDrawCalls.clear();
	this->animatingDrawCalls.clear();
	this->position = position;

	// Add empty mesh instance for air.
	this->addMeshInstance(SceneGraphVoxelMeshInstance());
}

void SceneGraphChunk::update()
{
	DebugNotImplemented();
}

SceneGraphVoxelMeshInstanceID SceneGraphChunk::addMeshInstance(SceneGraphVoxelMeshInstance &&meshInst)
{
	const SceneGraphVoxelMeshInstanceID id = static_cast<SceneGraphVoxelMeshInstanceID>(this->meshInsts.size());
	this->meshInsts.emplace_back(std::move(meshInst));
	return id;
}

void SceneGraphChunk::freeBuffers(RendererSystem3D &renderer)
{
	for (SceneGraphVoxelMeshInstance &meshInst : this->meshInsts)
	{
		meshInst.freeBuffers(renderer);
	}
}
