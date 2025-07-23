#ifndef VOXEL_BOX_COMBINE_CHUNK_MANAGER_H
#define VOXEL_BOX_COMBINE_CHUNK_MANAGER_H

#include "VoxelBoxCombineChunk.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Span.h"

class VoxelChunkManager;

// Combines voxel shapes where possible within each chunk for reduced collider count.
class VoxelBoxCombineChunkManager final : public SpecializedChunkManager<VoxelBoxCombineChunk>
{
public:
	void updateActiveChunks(Span<const ChunkInt2> newChunkPositions, Span<const ChunkInt2> freedChunkPositions, const VoxelChunkManager &voxelChunkManager);
	void update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions, const VoxelChunkManager &voxelChunkManager);
};

#endif
