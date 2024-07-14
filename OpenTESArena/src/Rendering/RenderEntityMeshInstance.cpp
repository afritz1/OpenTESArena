#include "RenderEntityMeshInstance.h"
#include "Renderer.h"

RenderEntityMeshInstance::RenderEntityMeshInstance()
{
	this->vertexBufferID = -1;
	this->normalBufferID = -1;
	this->texCoordBufferID = -1;
	this->indexBufferID = -1;
}

void RenderEntityMeshInstance::freeBuffers(Renderer &renderer)
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

	if (this->indexBufferID >= 0)
	{
		renderer.freeIndexBuffer(this->indexBufferID);
		this->indexBufferID = -1;
	}
}
