#ifndef VOXEL_FACE_COMBINE_CHUNK_MANAGER_H
#define VOXEL_FACE_COMBINE_CHUNK_MANAGER_H

#include "VoxelFaceCombineChunk.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Span.h"

class VoxelChunkManager;
class VoxelFaceEnableChunkManager;

// Combines voxel faces where possible within each chunk for reduced draw calls.
class VoxelFaceCombineChunkManager final : public SpecializedChunkManager<VoxelFaceCombineChunk>
{
public:
	void updateActiveChunks(Span<const ChunkInt2> newChunkPositions, Span<const ChunkInt2> freedChunkPositions,
		const VoxelChunkManager &voxelChunkManager);
	void update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
		const VoxelChunkManager &voxelChunkManager, const VoxelFaceEnableChunkManager &voxelFaceEnableChunkManager);

	void endFrame();
};

#endif
