#ifndef RENDER_VOXEL_CHUNK_H
#define RENDER_VOXEL_CHUNK_H

#include <unordered_map>
#include <vector>

#include "RenderDrawCall.h"
#include "RenderGeometryUtils.h"
#include "RenderShaderUtils.h"
#include "RenderVoxelMeshInstance.h"
#include "../Voxels/VoxelChunk.h"
#include "../Voxels/VoxelUtils.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"

class Renderer;

struct RenderVoxelDrawCallRange
{
	int index; // Index into the chunk's draw calls.
	int8_t count; // Number of draw calls for this voxel.

	RenderVoxelDrawCallRange();

	void clear();
};

using RenderVoxelDrawCallRangeID = int;

struct RenderVoxelDrawCallHeap
{
	static constexpr int MAX_DRAW_CALLS = 24000;
	static constexpr int MAX_DRAW_CALL_RANGES = (7 * MAX_DRAW_CALLS) / 8; // Arbitrary, most ranges will be 1 draw call.

	RenderDrawCall drawCalls[MAX_DRAW_CALLS];
	std::vector<int> freedDrawCalls;
	int nextDrawCall;

	RenderVoxelDrawCallRange drawCallRanges[MAX_DRAW_CALL_RANGES];
	std::vector<RenderVoxelDrawCallRangeID> freedIDs;
	RenderVoxelDrawCallRangeID nextID;

	RenderVoxelDrawCallHeap();

	BufferView<RenderDrawCall> get(RenderVoxelDrawCallRangeID id);
	BufferView<const RenderDrawCall> get(RenderVoxelDrawCallRangeID id) const;
	RenderVoxelDrawCallRangeID alloc(int drawCallCount);
	RenderVoxelDrawCallRangeID addDrawCall(RenderVoxelDrawCallRangeID id); // @temp hack to add one draw call more conveniently
	void free(RenderVoxelDrawCallRangeID id);
	void clear();
};

class RenderVoxelChunk final : public Chunk
{
public:
	static constexpr RenderVoxelMeshInstID AIR_MESH_INST_ID = 0;

	std::vector<RenderVoxelMeshInstance> meshInsts;
	std::unordered_map<VoxelChunk::VoxelMeshDefID, RenderVoxelMeshInstID> meshInstMappings; // Note: this doesn't support VoxelIDs changing which def they point to (important if VoxelChunk::removeVoxelDef() is ever in use).
	Buffer3D<RenderVoxelMeshInstID> meshInstIDs; // Points into mesh instances.
	std::unordered_map<VoxelInt3, IndexBufferID> chasmWallIndexBufferIDsMap; // If an index buffer ID exists for a voxel, it adds a draw call for the chasm wall. IDs are owned by the render chunk manager.
	std::unordered_map<VoxelInt3, UniformBufferID> transformBuffers; // Unique transform buffer per voxel, owned by this chunk. One RenderTransform per buffer.
	std::unordered_map<VoxelInt3, UniformBufferID> doorTransformBuffers; // Unique transform buffer per door instance, owned by this chunk. Four RenderTransforms (one per door face) per buffer.

	RenderVoxelDrawCallHeap drawCallHeap;
	Buffer3D<RenderVoxelDrawCallRangeID> staticDrawCallRangeIDs; // Most voxel geometry (walls, floors, etc.).
	Buffer3D<RenderVoxelDrawCallRangeID> doorDrawCallRangeIDs; // All doors, open or closed.
	Buffer3D<RenderVoxelDrawCallRangeID> chasmDrawCallRangeIDs; // Chasm walls and floors, separate from static draw calls so their textures can animate.
	Buffer3D<RenderVoxelDrawCallRangeID> fadingDrawCallRangeIDs; // Voxels with fade shader. Note that the static draw call in the same voxel needs to be deleted to avoid a conflict in the depth buffer.

	void init(const ChunkInt2 &position, int height);
	RenderVoxelMeshInstID addMeshInst(RenderVoxelMeshInstance &&meshInst);
	void freeStaticDrawCalls();
	void freeAnimatingDrawCalls();
	void freeBuffers(Renderer &renderer);
	void clear();
};

#endif
