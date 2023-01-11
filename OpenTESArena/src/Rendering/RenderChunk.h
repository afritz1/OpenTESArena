#ifndef RENDER_CHUNK_H
#define RENDER_CHUNK_H

#include <unordered_map>

#include "RenderDrawCall.h"
#include "RenderGeometryUtils.h"
#include "RenderVoxelMeshDefinition.h"
#include "../Voxels/VoxelChunk.h"
#include "../Voxels/VoxelUtils.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer3D.h"

class EntityManager;
class Renderer;

using RenderVoxelMeshDefID = int;

class RenderChunk final : public Chunk
{
public:
	static constexpr RenderVoxelMeshDefID AIR_MESH_DEF_ID = 0;

	std::vector<RenderVoxelMeshDefinition> meshDefs;
	std::unordered_map<VoxelChunk::VoxelMeshDefID, RenderVoxelMeshDefID> meshDefMappings; // Note: this doesn't support VoxelIDs changing which def they point to (important if VoxelChunk::removeVoxelDef() is ever in use).
	Buffer3D<RenderVoxelMeshDefID> meshDefIDs; // Points into mesh instances.
	std::unordered_map<VoxelInt3, IndexBufferID> chasmWallIndexBufferIDs; // If an index buffer ID exists for a voxel, it adds a draw call for the chasm wall. IDs are owned by the render chunk manager.
	std::vector<RenderDrawCall> staticDrawCalls; // Most voxel geometry (walls, floors, etc.).
	std::vector<RenderDrawCall> doorDrawCalls; // All doors, open or closed.
	std::vector<RenderDrawCall> chasmDrawCalls; // Chasm walls and floors, separate from static draw calls so their textures can animate.
	std::vector<RenderDrawCall> fadingDrawCalls; // Voxels with fade shader. Note that the static draw call in the same voxel needs to be deleted to avoid a conflict in the depth buffer.

	// @todo: quadtree
	// - thinking that each 'visible slice' of the tree could be a BufferView2D maybe, or a VoxelInt2 begin + end pattern

	// @todo: bounding box for "the whole chunk", update every frame. This box can be larger than the chunk because of entity overhang.
	// @todo: entities (stores vertex buffer id, etc. as well as transform and bounding box)

	void init(const ChunkInt2 &position, int height);
	RenderVoxelMeshDefID addMeshDefinition(RenderVoxelMeshDefinition &&meshDef);
	void freeBuffers(Renderer &renderer);
	void clear();
};

#endif
