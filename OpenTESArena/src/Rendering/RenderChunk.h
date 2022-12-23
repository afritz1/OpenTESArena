#ifndef RENDER_CHUNK_H
#define RENDER_CHUNK_H

#include <unordered_map>

#include "RenderChunkMesh.h"
#include "RenderDrawCall.h"
#include "RenderGeometryUtils.h"
#include "../Voxels/VoxelChunk.h"
#include "../Voxels/VoxelUtils.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer3D.h"

class EntityManager;
class Renderer;

using RenderChunkVoxelMeshInstanceID = int;

class RenderChunk final : public Chunk
{
public:
	std::vector<RenderChunkVoxelMeshInstance> meshInsts;
	std::unordered_map<VoxelChunk::VoxelMeshDefID, RenderChunkVoxelMeshInstanceID> meshInstMappings; // Note: this doesn't support VoxelIDs changing which def they point to (important if VoxelChunk::removeVoxelDef() is ever in use).
	Buffer3D<RenderChunkVoxelMeshInstanceID> meshInstIDs; // Points into mesh instances.
	std::unordered_map<VoxelInt3, IndexBufferID> chasmWallIndexBufferIDs; // If an index buffer ID exists for a voxel, it adds a draw call for the chasm wall. IDs are owned by the render chunk manager.
	std::vector<RenderDrawCall> staticDrawCalls; // Most voxel geometry.
	std::vector<RenderDrawCall> animatingDrawCalls; // Chasm floors, chasm walls, doors, etc. so their textures can animate.

	// @todo: quadtree
	// - thinking that each 'visible slice' of the tree could be a BufferView2D maybe, or a VoxelInt2 begin + end pattern

	// @todo: bounding box for "the whole chunk", update every frame. This box can be larger than the chunk because of entity overhang.
	// @todo: entities (stores vertex buffer id, etc. as well as transform and bounding box)

	void init(const ChunkInt2 &position, int height);
	void update();
	RenderChunkVoxelMeshInstanceID addMeshInstance(RenderChunkVoxelMeshInstance &&meshInst);
	void freeBuffers(Renderer &renderer);
};

#endif
