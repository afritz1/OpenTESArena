#ifndef RENDER_ENTITY_MESH_INSTANCE_H
#define RENDER_ENTITY_MESH_INSTANCE_H

#include "RenderMeshUtils.h"

class Renderer;

// All the resources needed to define an entity's renderer-allocated mesh.
struct RenderEntityMeshInstance
{
	VertexPositionBufferID positionBufferID;
	VertexAttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID indexBufferID;

	RenderEntityMeshInstance();

	void freeBuffers(Renderer &renderer);
};

#endif
