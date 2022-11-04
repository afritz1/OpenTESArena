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

	// @todo: index buffers for voxel instances (i.e. chasm walls) will likely be separately stored in the scene graph like a default + override

	SceneGraphVoxelMeshInstance();

	void freeBuffers(RendererSystem3D &renderer3D);
};

// @todo: should there be a SceneGraphChasmMeshInstance? maybe rename the one above while we're at it
// - the chasm mesh should be similar to a wall mesh; whatever's needed to support 5 individual faces and 2 textures (one of which is an animation)

#endif
