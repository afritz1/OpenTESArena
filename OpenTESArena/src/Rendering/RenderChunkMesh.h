#ifndef RENDER_CHUNK_MESH_H
#define RENDER_CHUNK_MESH_H

#include "RenderGeometryUtils.h"

class Renderer;

struct RenderChunkVoxelMeshInstance
{
	static constexpr int MAX_TEXTURES = 3; // Based on VoxelDefinition subtypes (wall and raised).

	VertexBufferID vertexBufferID;
	AttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID opaqueIndexBufferIDs[MAX_TEXTURES];
	int opaqueIndexBufferIdCount;
	IndexBufferID alphaTestedIndexBufferID;

	RenderChunkVoxelMeshInstance();

	void freeBuffers(Renderer &renderer);
};

#endif
