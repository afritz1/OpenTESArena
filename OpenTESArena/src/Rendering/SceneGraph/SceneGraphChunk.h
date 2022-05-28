#ifndef SCENE_GRAPH_CHUNK_H
#define SCENE_GRAPH_CHUNK_H

#include <unordered_map>

#include "../RenderGeometryUtils.h"
#include "../RenderTriangle.h"
#include "../../World/Chunk.h"
#include "../../World/VoxelUtils.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer3D.h"

class EntityManager;
class RendererSystem3D;

struct SceneGraphVoxelDefinition
{
	VertexBufferID vertexBufferID;
	AttributeBufferID attributeBufferID;
	IndexBufferID opaqueIndexBufferID, alphaTestedIndexBufferID;
	// @todo: index buffers for voxel instances (i.e. chasm walls) will likely be separately stored in the scene graph like a default + override

	SceneGraphVoxelDefinition();

	void freeBuffers(RendererSystem3D &renderer3D);
};

using SceneGraphVoxelID = int;

struct SceneGraphChunk
{
	std::vector<SceneGraphVoxelDefinition> voxelDefs;
	std::unordered_map<Chunk::VoxelID, SceneGraphVoxelID> voxelDefMappings; // Note: this doesn't support VoxelIDs changing which def they point to (important if Chunk::removeVoxelDef() is ever in use).
	Buffer3D<SceneGraphVoxelID> voxels; // Points into voxel defs.
	ChunkInt2 position;

	// @todo: quadtree
	// - thinking that each 'visible slice' of the tree could be a BufferView2D maybe, or a VoxelInt2 begin + end pattern

	// @todo: bounding box for "the whole chunk", update every frame. This box can be larger than the chunk because of entity overhang.
	// @todo: scene graph entities (stores vertex buffer id, etc. as well as transform and bounding box)

	void init(const ChunkInt2 &position, int height);
	void update(EntityManager &entityManager);
	void freeBuffers(RendererSystem3D &renderer);
};

#endif
