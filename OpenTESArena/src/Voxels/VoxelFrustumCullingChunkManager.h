#ifndef VOXEL_FRUSTUM_CULLING_CHUNK_MANAGER_H
#define VOXEL_FRUSTUM_CULLING_CHUNK_MANAGER_H

#include "VoxelFrustumCullingChunk.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/BufferView.h"

class VoxelChunkManager;

struct RenderCamera;

class VoxelFrustumCullingChunkManager final : public SpecializedChunkManager<VoxelFrustumCullingChunk>
{
public:
	void update(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
		const RenderCamera &camera, double ceilingScale, const VoxelChunkManager &voxelChunkManager);
};

#endif
