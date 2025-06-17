#ifndef RENDER_LIGHT_CHUNK_H
#define RENDER_LIGHT_CHUNK_H

#include <vector>

#include "RenderDrawCall.h"
#include "RenderLightUtils.h"
#include "../Voxels/VoxelUtils.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer3D.h"
#include "components/utilities/Span.h"

class Renderer;

struct RenderLightChunk final : public Chunk
{
	Buffer3D<RenderLightIdList> lightIdLists; // Enabled lights touching each voxel. IDs are owned by RenderLightChunkManager.
	
	std::vector<VoxelInt3> dirtyVoxelPositions; // Voxels with added/removed light IDs this frame that the renderer should update draw calls for.
	Buffer3D<bool> dirtyVoxels; // Optimization for dirty voxel position lookup.

	void init(const ChunkInt2 &position, int height);
	void setVoxelDirty(const VoxelInt3 &position);
	void clearDirtyVoxels();
	void clear();
};

#endif
