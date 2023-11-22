#ifndef RENDER_VOXEL_MESH_INSTANCE_H
#define RENDER_VOXEL_MESH_INSTANCE_H

#include "RenderGeometryUtils.h"

class Renderer;

using RenderVoxelMeshInstID = int;

struct RenderVoxelMeshInstance
{
	static constexpr int MAX_TEXTURES = 3; // Based on VoxelDefinition subtypes (wall and raised).

	VertexBufferID vertexBufferID;
	AttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID opaqueIndexBufferIDs[MAX_TEXTURES];
	int opaqueIndexBufferIdCount;
	IndexBufferID alphaTestedIndexBufferID;

	RenderVoxelMeshInstance();

	int getTotalDrawCallCount() const;
	void freeBuffers(Renderer &renderer);
};

#endif
