#ifndef SCENE_GRAPH_MESH_H
#define SCENE_GRAPH_MESH_H

#include "../RenderGeometryUtils.h"

class RendererSystem3D;

struct SceneGraphVoxelMeshInstance
{
	static constexpr int MAX_TEXTURES = 3; // Based on VoxelDefinition subtypes (wall and raised).

	VertexBufferID vertexBufferID;
	AttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID opaqueIndexBufferIDs[MAX_TEXTURES];
	int opaqueIndexBufferIdCount;
	IndexBufferID alphaTestedIndexBufferID;

	SceneGraphVoxelMeshInstance();

	void freeBuffers(RendererSystem3D &renderer3D);
};

#endif
