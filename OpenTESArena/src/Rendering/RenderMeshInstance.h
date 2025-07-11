#ifndef RENDER_MESH_INSTANCE_H
#define RENDER_MESH_INSTANCE_H

#include "RenderMeshUtils.h"

class Renderer;

using RenderVoxelMeshInstID = int;

struct RenderVoxelMeshInstance
{
	VertexPositionBufferID positionBufferID;
	VertexAttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID indexBufferID;

	RenderVoxelMeshInstance();

	void freeBuffers(Renderer &renderer);
};

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
