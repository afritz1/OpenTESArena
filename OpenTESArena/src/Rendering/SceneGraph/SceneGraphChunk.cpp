#include <algorithm>

#include "SceneGraphChunk.h"
#include "../RendererSystem3D.h"
#include "../../World/ChunkUtils.h"

void SceneGraphChunk::init(const ChunkInt2 &position, int height)
{
	this->meshInsts.clear();
	this->meshInstMappings.clear();
	this->meshInstIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->voxelDrawCalls.clear();
	this->position = position;

	// Add empty mesh instance for air.
	this->addMeshInstance(SceneGraphVoxelMeshInstance());
}

void SceneGraphChunk::update(EntityManager &entityManager)
{
	// @todo: refresh bounding box for the whole chunk; needs to iterate all entities in the EntityManager's chunk every frame.
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
