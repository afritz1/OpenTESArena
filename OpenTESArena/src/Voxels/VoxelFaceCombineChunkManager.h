#ifndef VOXEL_FACE_COMBINE_CHUNK_MANAGER_H
#define VOXEL_FACE_COMBINE_CHUNK_MANAGER_H

#include "VoxelFaceCombineChunk.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/BufferView.h"

class VoxelChunkManager;
class VoxelFaceEnableChunkManager;

// Combines voxel faces where possible within each chunk for reduced draw calls.
class VoxelFaceCombineChunkManager : public SpecializedChunkManager<VoxelFaceCombineChunk>
{
public:
	void updateActiveChunks(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
		const VoxelChunkManager &voxelChunkManager);
	void update(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
		const VoxelChunkManager &voxelChunkManager, const VoxelFaceEnableChunkManager &voxelFaceEnableChunkManager);
};

#endif
