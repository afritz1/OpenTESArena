#ifndef RENDER_VOXEL_MESH_INSTANCE_H
#define RENDER_VOXEL_MESH_INSTANCE_H

#include "RenderGeometryUtils.h"

class Renderer;

using RenderVoxelMeshInstID = int;

struct RenderVoxelMeshInstance
{
	static constexpr int MAX_DRAW_CALLS = 3; // Based on VoxelDefinition subtypes (wall or raised).

	VertexBufferID vertexBufferID;
	AttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID indexBufferIDs[MAX_DRAW_CALLS];
	int indexBufferIdCount;

	RenderVoxelMeshInstance();

	int getUniqueDrawCallCount() const;
	void freeBuffers(Renderer &renderer);
};

#endif
