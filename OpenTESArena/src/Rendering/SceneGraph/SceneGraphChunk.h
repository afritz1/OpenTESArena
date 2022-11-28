#ifndef SCENE_GRAPH_CHUNK_H
#define SCENE_GRAPH_CHUNK_H

#include <unordered_map>

#include "SceneGraphMesh.h"
#include "../RenderDrawCall.h"
#include "../RenderGeometryUtils.h"
#include "../../Voxels/VoxelChunk.h"
#include "../../Voxels/VoxelUtils.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer3D.h"

class EntityManager;
class RendererSystem3D;

using SceneGraphVoxelMeshInstanceID = int;

struct SceneGraphChunk
{
	std::vector<SceneGraphVoxelMeshInstance> meshInsts;
	std::unordered_map<VoxelChunk::VoxelMeshDefID, SceneGraphVoxelMeshInstanceID> meshInstMappings; // Note: this doesn't support VoxelIDs changing which def they point to (important if VoxelChunk::removeVoxelDef() is ever in use).
	Buffer3D<SceneGraphVoxelMeshInstanceID> meshInstIDs; // Points into mesh instances.
	std::unordered_map<VoxelInt3, IndexBufferID> chasmWallIndexBufferIDs; // If an index buffer ID exists for a voxel, it adds a draw call for the chasm wall. IDs are owned by the scene graph.
	std::vector<RenderDrawCall> staticDrawCalls; // Most voxel geometry.
	std::vector<RenderDrawCall> animatingDrawCalls; // Chasm floors, chasm walls, doors, etc. so their textures can animate.
	ChunkInt2 position;

	// @todo: quadtree
	// - thinking that each 'visible slice' of the tree could be a BufferView2D maybe, or a VoxelInt2 begin + end pattern

	// @todo: bounding box for "the whole chunk", update every frame. This box can be larger than the chunk because of entity overhang.
	// @todo: scene graph entities (stores vertex buffer id, etc. as well as transform and bounding box)

	void init(const ChunkInt2 &position, int height);
	void update();
	SceneGraphVoxelMeshInstanceID addMeshInstance(SceneGraphVoxelMeshInstance &&meshInst);
	void freeBuffers(RendererSystem3D &renderer);
};

#endif
