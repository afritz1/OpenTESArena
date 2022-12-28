#ifndef RENDER_VOXEL_MESH_DEFINITION_H
#define RENDER_VOXEL_MESH_DEFINITION_H

#include "RenderGeometryUtils.h"

class Renderer;

struct RenderVoxelMeshDefinition
{
	static constexpr int MAX_TEXTURES = 3; // Based on VoxelDefinition subtypes (wall and raised).

	VertexBufferID vertexBufferID;
	AttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID opaqueIndexBufferIDs[MAX_TEXTURES];
	int opaqueIndexBufferIdCount;
	IndexBufferID alphaTestedIndexBufferID;

	RenderVoxelMeshDefinition();

	void freeBuffers(Renderer &renderer);
};

#endif
