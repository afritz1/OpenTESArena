#ifndef VOXEL_VISIBILITY_CHUNK_MANAGER_H
#define VOXEL_VISIBILITY_CHUNK_MANAGER_H

#include "VoxelVisibilityChunk.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/BufferView.h"

class VoxelChunkManager;

struct RenderCamera;

class VoxelVisibilityChunkManager final : public SpecializedChunkManager<VoxelVisibilityChunk>
{
public:
	void update(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
		const RenderCamera &camera, double ceilingScale, const VoxelChunkManager &voxelChunkManager);
};

#endif
