#include <algorithm>

#include "RenderChunkMesh.h"
#include "RendererSystem3D.h"

RenderChunkVoxelMeshInstance::RenderChunkVoxelMeshInstance()
{
	this->vertexBufferID = -1;
	this->normalBufferID = -1;
	this->texCoordBufferID = -1;
	std::fill(std::begin(this->opaqueIndexBufferIDs), std::end(this->opaqueIndexBufferIDs), -1);
	this->opaqueIndexBufferIdCount = 0;
	this->alphaTestedIndexBufferID = -1;
}

void RenderChunkVoxelMeshInstance::freeBuffers(RendererSystem3D &renderer3D)
{
	if (this->vertexBufferID >= 0)
	{
		renderer3D.freeVertexBuffer(this->vertexBufferID);
		this->vertexBufferID = -1;
	}

	if (this->normalBufferID >= 0)
	{
		renderer3D.freeAttributeBuffer(this->normalBufferID);
		this->normalBufferID = -1;
	}

	if (this->texCoordBufferID >= 0)
	{
		renderer3D.freeAttributeBuffer(this->texCoordBufferID);
		this->texCoordBufferID = -1;
	}

	if (this->opaqueIndexBufferIdCount > 0)
	{
		for (int i = 0; i < this->opaqueIndexBufferIdCount; i++)
		{
			renderer3D.freeIndexBuffer(this->opaqueIndexBufferIDs[i]);
		}

		std::fill(std::begin(this->opaqueIndexBufferIDs), std::end(this->opaqueIndexBufferIDs), -1);
		this->opaqueIndexBufferIdCount = 0;
	}

	if (this->alphaTestedIndexBufferID >= 0)
	{
		renderer3D.freeIndexBuffer(this->alphaTestedIndexBufferID);
		this->alphaTestedIndexBufferID = -1;
	}
}
