#ifndef RENDER_ENTITY_MESH_DEFINITION_H
#define RENDER_ENTITY_MESH_DEFINITION_H

#include "RenderGeometryUtils.h"

class Renderer;

// All the resources needed to define an entity's renderer-allocated mesh.
struct RenderEntityMeshDefinition
{
	VertexBufferID vertexBufferID;
	AttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID indexBufferID;

	RenderEntityMeshDefinition();

	void freeBuffers(Renderer &renderer);
};

#endif
