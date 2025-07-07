#ifndef RENDER_MESH_INSTANCE_H
#define RENDER_MESH_INSTANCE_H

#include "RenderMeshUtils.h"

class Renderer;

using RenderVoxelMeshInstID = int;

struct RenderVoxelMeshInstance
{
	static constexpr int MAX_DRAW_CALLS = 6; // Based on voxel mesh faces.

	VertexPositionBufferID positionBufferID;
	VertexAttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID indexBufferIDs[MAX_DRAW_CALLS];
	int indexBufferIdCount;

	RenderVoxelMeshInstance();

	int getUniqueDrawCallCount() const;
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
