#ifndef RENDER_VOXEL_CHUNK_H
#define RENDER_VOXEL_CHUNK_H

#include <unordered_map>
#include <vector>

#include "RenderDrawCall.h"
#include "RenderMeshInstance.h"
#include "RenderMeshUtils.h"
#include "RenderShaderUtils.h"
#include "../Voxels/VoxelChunk.h"
#include "../Voxels/VoxelFaceCombineChunk.h"
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

struct RenderVoxelCombinedFaceTransformKey
{
	VoxelInt3 minVoxel, maxVoxel;
	VoxelFacing3D facing;

	RenderVoxelCombinedFaceTransformKey();

	bool operator==(const RenderVoxelCombinedFaceTransformKey &other) const;
};

struct RenderVoxelCombinedFaceDrawCallEntry
{
	RenderVoxelDrawCallRangeID rangeID; // One draw call.
	VoxelInt3 min, max;

	RenderVoxelCombinedFaceDrawCallEntry();
};

namespace std
{
	// For fast lookup of a mesh instance's transform in this chunk.
	template<>
	struct hash<RenderVoxelCombinedFaceTransformKey>
	{
		size_t operator()(const RenderVoxelCombinedFaceTransformKey &key) const
		{
			const size_t minVoxelHash = key.minVoxel.toHash();
			const size_t maxVoxelHash = key.maxVoxel.toHash();
			const size_t facingHash = static_cast<size_t>(key.facing);

			size_t hash = 0;
			hash = MathUtils::hashCombine(hash, minVoxelHash);
			hash = MathUtils::hashCombine(hash, maxVoxelHash);
			hash = MathUtils::hashCombine(hash, facingHash);
			return hash;
		}
	};
}

struct RenderVoxelChunk final : public Chunk
{
	static constexpr RenderMeshInstID AIR_MESH_INST_ID = 0;

	std::vector<RenderMeshInstance> meshInsts;
	std::unordered_map<VoxelShapeDefID, RenderMeshInstID> meshInstMappings;

	std::vector<RenderVoxelCombinedFaceDrawCallEntry> combinedFaceDrawCallEntries;
	std::unordered_map<RenderVoxelCombinedFaceTransformKey, UniformBufferID> combinedFaceTransforms; // Allocated transforms for static positions in space, doesn't need freeing when dirty.

	UniformBufferID transformBufferID; // One RenderTransform buffer for all voxels, though doors are handled separately. Owned by this chunk.
	std::unordered_map<VoxelInt3, UniformBufferID> doorTransformBuffers; // Unique transform buffer per door instance, owned by this chunk. Four RenderTransforms (one per door face) per buffer.

	RenderVoxelDrawCallHeap drawCallHeap;
	Buffer3D<RenderVoxelDrawCallRangeID> drawCallRangeIDs; // Most voxel geometry (walls, floors, etc.).

	void init(const ChunkInt2 &position, int height);
	RenderMeshInstID addMeshInst(RenderMeshInstance &&meshInst);
	void freeDrawCalls(SNInt x, int y, WEInt z);
	void freeBuffers(Renderer &renderer);
	void clear();
};

#endif
