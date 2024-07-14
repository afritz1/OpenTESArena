#include <algorithm>

#include "RenderVoxelMeshInstance.h"
#include "Renderer.h"

RenderVoxelMeshInstance::RenderVoxelMeshInstance()
{
	this->vertexBufferID = -1;
	this->normalBufferID = -1;
	this->texCoordBufferID = -1;
	std::fill(std::begin(this->opaqueIndexBufferIDs), std::end(this->opaqueIndexBufferIDs), -1);
	this->opaqueIndexBufferIdCount = 0;
	this->alphaTestedIndexBufferID = -1;
}

int RenderVoxelMeshInstance::getUniqueDrawCallCount() const
{
	int count = this->opaqueIndexBufferIdCount;
	if (this->alphaTestedIndexBufferID >= 0)
	{
		count++;
	}

	return count;
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

	if (this->opaqueIndexBufferIdCount > 0)
	{
		for (int i = 0; i < this->opaqueIndexBufferIdCount; i++)
		{
			renderer.freeIndexBuffer(this->opaqueIndexBufferIDs[i]);
		}

		std::fill(std::begin(this->opaqueIndexBufferIDs), std::end(this->opaqueIndexBufferIDs), -1);
		this->opaqueIndexBufferIdCount = 0;
	}

	if (this->alphaTestedIndexBufferID >= 0)
	{
		renderer.freeIndexBuffer(this->alphaTestedIndexBufferID);
		this->alphaTestedIndexBufferID = -1;
	}
}
