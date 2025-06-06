#include <algorithm>

#include "RenderMeshInstance.h"
#include "Renderer.h"

RenderVoxelMeshInstance::RenderVoxelMeshInstance()
{
	this->positionBufferID = -1;
	this->normalBufferID = -1;
	this->texCoordBufferID = -1;
	std::fill(std::begin(this->indexBufferIDs), std::end(this->indexBufferIDs), -1);
	this->indexBufferIdCount = 0;
}

int RenderVoxelMeshInstance::getUniqueDrawCallCount() const
{
	return this->indexBufferIdCount;
}

void RenderVoxelMeshInstance::freeBuffers(Renderer &renderer)
{
	if (this->positionBufferID >= 0)
	{
		renderer.freeVertexPositionBuffer(this->positionBufferID);
		this->positionBufferID = -1;
	}

	if (this->normalBufferID >= 0)
	{
		renderer.freeVertexAttributeBuffer(this->normalBufferID);
		this->normalBufferID = -1;
	}

	if (this->texCoordBufferID >= 0)
	{
		renderer.freeVertexAttributeBuffer(this->texCoordBufferID);
		this->texCoordBufferID = -1;
	}

	for (int i = 0; i < this->indexBufferIdCount; i++)
	{
		renderer.freeIndexBuffer(this->indexBufferIDs[i]);
	}

	std::fill(std::begin(this->indexBufferIDs), std::end(this->indexBufferIDs), -1);
	this->indexBufferIdCount = 0;
}

RenderEntityMeshInstance::RenderEntityMeshInstance()
{
	this->positionBufferID = -1;
	this->normalBufferID = -1;
	this->texCoordBufferID = -1;
	this->indexBufferID = -1;
}

void RenderEntityMeshInstance::freeBuffers(Renderer &renderer)
{
	if (this->positionBufferID >= 0)
	{
		renderer.freeVertexPositionBuffer(this->positionBufferID);
		this->positionBufferID = -1;
	}

	if (this->normalBufferID >= 0)
	{
		renderer.freeVertexAttributeBuffer(this->normalBufferID);
		this->normalBufferID = -1;
	}

	if (this->texCoordBufferID >= 0)
	{
		renderer.freeVertexAttributeBuffer(this->texCoordBufferID);
		this->texCoordBufferID = -1;
	}

	if (this->indexBufferID >= 0)
	{
		renderer.freeIndexBuffer(this->indexBufferID);
		this->indexBufferID = -1;
	}
}
