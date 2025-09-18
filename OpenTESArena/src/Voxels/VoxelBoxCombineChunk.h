#ifndef VOXEL_BOX_COMBINE_CHUNK_H
#define VOXEL_BOX_COMBINE_CHUNK_H

#include <vector>

#include "VoxelUtils.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"
#include "components/utilities/KeyValuePool.h"
#include "components/utilities/Span.h"

struct VoxelChunk;

// One or more adjacent voxel shapes combined into a larger shape.
struct VoxelBoxCombineResult
{
	VoxelInt3 min, max; // Inclusive max.

	VoxelBoxCombineResult();

	void clear();
};

using VoxelBoxCombineResultID = int;

struct VoxelBoxCombineChunk final : public Chunk
{
private:
	Buffer3D<bool> dirtyEntries; // Boxes marked for rebuilding this frame.
	std::vector<VoxelInt3> dirtyEntryPositions; // Voxels that need dirty entry updating this frame. Cleared at start of each update.
public:
	KeyValuePool<VoxelBoxCombineResultID, VoxelBoxCombineResult> combinedBoxesPool;
	Buffer3D<VoxelBoxCombineResultID> entryIDs;

	void init(const ChunkInt2 &position, int height);

	void update(Span<const VoxelInt3> dirtyVoxels, const VoxelChunk &voxelChunk);

	void clear();
};

#endif
