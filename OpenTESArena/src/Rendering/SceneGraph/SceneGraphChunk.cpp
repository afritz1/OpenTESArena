#include <algorithm>

#include "SceneGraphChunk.h"
#include "../RendererSystem3D.h"
#include "../../World/ChunkUtils.h"

SceneGraphVoxelDefinition::SceneGraphVoxelDefinition()
{
	this->vertexBufferID = -1;
	this->attributeBufferID = -1;
	this->opaqueIndexBufferID = -1;
	this->alphaTestedIndexBufferID = -1;
}

void SceneGraphVoxelDefinition::freeBuffers(RendererSystem3D &renderer3D)
{
	if (this->vertexBufferID >= 0)
	{
		renderer3D.freeVertexBuffer(this->vertexBufferID);
		this->vertexBufferID = -1;
	}

	if (this->attributeBufferID >= 0)
	{
		renderer3D.freeAttributeBuffer(this->attributeBufferID);
		this->attributeBufferID = -1;
	}

	if (this->opaqueIndexBufferID >= 0)
	{
		renderer3D.freeIndexBuffer(this->opaqueIndexBufferID);
		this->opaqueIndexBufferID = -1;
	}

	if (this->alphaTestedIndexBufferID >= 0)
	{
		renderer3D.freeIndexBuffer(this->alphaTestedIndexBufferID);
		this->alphaTestedIndexBufferID = -1;
	}
}

void SceneGraphChunk::init(const ChunkInt2 &position, int height)
{
	this->voxelDefs.clear();
	this->voxelDefMappings.clear();
	this->voxels.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->position = position;

	// Add one voxel def for air.
	this->addVoxelDef(SceneGraphVoxelDefinition());
}

void SceneGraphChunk::update(EntityManager &entityManager)
{
	// @todo: refresh bounding box for the whole chunk; needs to iterate all entities in the EntityManager's chunk every frame.
	DebugNotImplemented();
}

SceneGraphVoxelID SceneGraphChunk::addVoxelDef(SceneGraphVoxelDefinition &&voxelDef)
{
	const SceneGraphVoxelID id = static_cast<SceneGraphVoxelID>(this->voxelDefs.size());
	this->voxelDefs.emplace_back(std::move(voxelDef));
	return id;
}

void SceneGraphChunk::freeBuffers(RendererSystem3D &renderer)
{
	std::for_each(this->voxelDefs.begin(), this->voxelDefs.end(), [&renderer](SceneGraphVoxelDefinition &voxelDef)
	{
		voxelDef.freeBuffers(renderer);
	});
}
