#ifndef RENDER_VOXEL_CHUNK_H
#define RENDER_VOXEL_CHUNK_H

#include <unordered_map>
#include <vector>

#include "RenderDrawCall.h"
#include "RenderMeshInstance.h"
#include "RenderMeshUtils.h"
#include "RenderShaderUtils.h"
#include "../Math/Matrix4.h"
#include "../Voxels/VoxelChunk.h"
#include "../Voxels/VoxelFaceCombineChunk.h"
#include "../Voxels/VoxelUtils.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"
#include "components/utilities/RecyclablePool.h"

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
	static constexpr int MAX_DRAW_CALLS = 8192;

	RenderDrawCall drawCalls[MAX_DRAW_CALLS];
	std::vector<int> freedDrawCalls;
	int nextDrawCall;

	RecyclablePool<RenderVoxelDrawCallRangeID, RenderVoxelDrawCallRange> drawCallRangesPool;

	RenderVoxelDrawCallHeap();

	Span<RenderDrawCall> get(RenderVoxelDrawCallRangeID id);
	Span<const RenderDrawCall> get(RenderVoxelDrawCallRangeID id) const;
	RenderVoxelDrawCallRangeID alloc(int drawCallCount);
	void free(RenderVoxelDrawCallRangeID id);
	void clear();
};

// One per uniform buffer.
struct RenderVoxelTransformHeap
{
	static constexpr int MAX_TRANSFORMS = 8192;

	UniformBufferID uniformBufferID;
	Matrix4d modelMatrices[MAX_TRANSFORMS]; // Copied into uniform buffer every frame.
	std::vector<int> freedMatrices;
	int nextMatrix;

	RenderVoxelTransformHeap();

	int alloc();
	void free(int transformIndex);
	void clear();
};

struct RenderVoxelCombinedFaceDrawCallEntry
{
	RenderVoxelDrawCallRangeID rangeID; // One draw call.
	int transformIndex; // Points into transform heap matrices.
	VoxelInt3 min, max;

	RenderVoxelCombinedFaceDrawCallEntry();
};

struct RenderVoxelNonCombinedTransformEntry
{
	VoxelInt3 voxel;
	UniformBufferID transformBufferID; // One model matrix.
	// @todo transform index allocated from transform heap

	RenderVoxelNonCombinedTransformEntry();
};

struct RenderVoxelDoorTransformsEntry
{
	VoxelInt3 voxel;
	UniformBufferID transformBufferID; // Four model matrices, one per face.
	// @todo 4 transform indices allocated from transform heap (don't have to be sequential)

	RenderVoxelDoorTransformsEntry();
};

struct RenderVoxelMaterialInstanceEntry
{
	VoxelInt3 voxel;
	RenderMaterialInstanceID materialInstID; // Allocated for whatever temporary effect is occurring (fading animation, door animation).

	RenderVoxelMaterialInstanceEntry();
};

struct RenderVoxelChunk final : public Chunk
{
	static constexpr RenderMeshInstID AIR_MESH_INST_ID = 0;

	std::vector<RenderMeshInstance> meshInsts;
	std::unordered_map<VoxelShapeDefID, RenderMeshInstID> meshInstMappings;

	// Mappings of combined face IDs to their draw call and transform.
	// @todo ideally this'd be a RecyclablePool for faster iteration but RenderVoxelChunkManager::updateChunkCombinedVoxelDrawCalls() would want an emplace(key, value) function which RecyclablePool doesn't have, it has alloc() which doesn't let us use the VoxelFaceCombineResultID from the loop
	// - RecyclablePool::emplace() would check that the given key is not used
	std::unordered_map<VoxelFaceCombineResultID, RenderVoxelCombinedFaceDrawCallEntry> combinedFaceDrawCallEntries;

	std::vector<RenderVoxelNonCombinedTransformEntry> nonCombinedTransformEntries; // One transform per non-combined voxel. Owned by this chunk.
	std::vector<RenderVoxelDoorTransformsEntry> doorTransformEntries; // Transforms for door voxel, owned by this chunk.
	
	RenderVoxelTransformHeap transformHeap;

	std::vector<RenderVoxelMaterialInstanceEntry> doorMaterialInstEntries;
	std::vector<RenderVoxelMaterialInstanceEntry> fadeMaterialInstEntries;

	RenderVoxelDrawCallHeap drawCallHeap;
	Buffer3D<RenderVoxelDrawCallRangeID> drawCallRangeIDs; // Non-combined voxel geometry (diagonals, doors).

	void init(const ChunkInt2 &position, int height);
	RenderMeshInstID addMeshInst(RenderMeshInstance &&meshInst);
	void freeDrawCalls(SNInt x, int y, WEInt z);
	void freeDoorMaterial(SNInt x, int y, WEInt z, Renderer &renderer);
	void freeFadeMaterial(SNInt x, int y, WEInt z, Renderer &renderer);
	void freeBuffers(Renderer &renderer);
	void clear();
};

#endif
