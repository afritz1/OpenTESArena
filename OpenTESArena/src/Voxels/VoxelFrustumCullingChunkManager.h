#pragma once

#include "VoxelFrustumCullingChunk.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Span.h"

class VoxelChunkManager;

struct RenderCamera;

class VoxelFrustumCullingChunkManager final : public SpecializedChunkManager<VoxelFrustumCullingChunk>
{
public:
	void update(Span<const ChunkInt2> newChunkPositions, Span<const ChunkInt2> freedChunkPositions,
		const RenderCamera &camera, double ceilingScale, const VoxelChunkManager &voxelChunkManager);
};
