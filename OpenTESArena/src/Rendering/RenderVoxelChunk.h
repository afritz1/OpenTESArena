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
#include "../Voxels/VoxelDoorUtils.h"
#include "../Voxels/VoxelFaceCombineChunk.h"
#include "../Voxels/VoxelUtils.h"
#include "../World/Chunk.h"

class Renderer;

struct RenderVoxelCombinedFaceDrawCallEntry
{
	VoxelInt3 min, max;
	int transformIndex; // Allocated from transform heap, points to model matrix.
	RenderDrawCall drawCall;

	RenderVoxelCombinedFaceDrawCallEntry();
};

struct RenderVoxelNonCombinedDrawCallEntry
{
	VoxelInt3 voxel;
	UniformBufferID transformBufferID; // One model matrix.
	// @todo transform index allocated from transform heap
	RenderDrawCall drawCall;

	RenderVoxelNonCombinedDrawCallEntry();
};

struct RenderVoxelDoorDrawCallsEntry
{
	VoxelInt3 voxel;
	ArenaDoorType doorType;
	int doorFaceIndices[VoxelDoorUtils::FACE_COUNT];
	UniformBufferID transformBufferID; // Four model matrices, one per face.
	// @todo 4 transform indices allocated from transform heap (don't have to be sequential)
	RenderDrawCall drawCalls[VoxelDoorUtils::FACE_COUNT];
	int drawCallCount;

	RenderVoxelDoorDrawCallsEntry();
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

	std::vector<RenderVoxelNonCombinedDrawCallEntry> nonCombinedDrawCallEntries; // One draw call + transform per non-combined voxel. Owned by this chunk.
	std::vector<RenderVoxelDoorDrawCallsEntry> doorDrawCallsEntries; // Draw calls + transforms for door voxel. Owned by this chunk.
	
	RenderTransformHeap transformHeap;

	std::vector<RenderVoxelMaterialInstanceEntry> doorMaterialInstEntries;
	std::vector<RenderVoxelMaterialInstanceEntry> fadeMaterialInstEntries;

	void init(const ChunkInt2 &position, int height);
	RenderMeshInstID addMeshInst(RenderMeshInstance &&meshInst);
	void freeDoorMaterial(SNInt x, int y, WEInt z, Renderer &renderer);
	void freeFadeMaterial(SNInt x, int y, WEInt z, Renderer &renderer);
	void freeBuffers(Renderer &renderer);
	void clear();
};

#endif
