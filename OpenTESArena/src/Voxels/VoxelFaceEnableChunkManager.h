#ifndef VOXEL_FACE_ENABLE_MANAGER_CHUNK_H
#define VOXEL_FACE_ENABLE_MANAGER_CHUNK_H

#include "VoxelFaceEnableChunk.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Span.h"

class VoxelChunkManager;

// Tracks which voxel faces within each chunk are internal faces blocked by opaque neighbor blocks.
class VoxelFaceEnableChunkManager final : public SpecializedChunkManager<VoxelFaceEnableChunk>
{
public:
	void updateActiveChunks(Span<const ChunkInt2> newChunkPositions, Span<const ChunkInt2> freedChunkPositions,
		const VoxelChunkManager &voxelChunkManager);
	void update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
		const VoxelChunkManager &voxelChunkManager);
};

#endif
