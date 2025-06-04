#ifndef VOXEL_FACE_ENABLE_MANAGER_CHUNK_H
#define VOXEL_FACE_ENABLE_MANAGER_CHUNK_H

#include "VoxelFaceEnableChunk.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/BufferView.h"

class VoxelChunkManager;

// Tracks which voxel faces within each chunk are internal faces blocked by opaque neighbor blocks.
class VoxelFaceEnableChunkManager : public SpecializedChunkManager<VoxelFaceEnableChunk>
{
public:
	void updateActiveChunks(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
		const VoxelChunkManager &voxelChunkManager);
	void update(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
		const VoxelChunkManager &voxelChunkManager);
};

#endif
