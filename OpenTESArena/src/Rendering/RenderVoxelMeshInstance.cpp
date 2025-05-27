#include <algorithm>

#include "RenderVoxelMeshInstance.h"
#include "Renderer.h"

RenderVoxelMeshInstance::RenderVoxelMeshInstance()
{
	this->vertexBufferID = -1;
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
	if (this->vertexBufferID >= 0)
	{
		renderer.freeVertexBuffer(this->vertexBufferID);
		this->vertexBufferID = -1;
	}

	if (this->normalBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->normalBufferID);
		this->normalBufferID = -1;
	}

	if (this->texCoordBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->texCoordBufferID);
		this->texCoordBufferID = -1;
	}

	for (int i = 0; i < this->indexBufferIdCount; i++)
	{
		renderer.freeIndexBuffer(this->indexBufferIDs[i]);
	}

	std::fill(std::begin(this->indexBufferIDs), std::end(this->indexBufferIDs), -1);
	this->indexBufferIdCount = 0;
}
