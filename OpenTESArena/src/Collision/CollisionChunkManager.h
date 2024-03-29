#ifndef COLLISION_CHUNK_MANAGER_H
#define COLLISION_CHUNK_MANAGER_H

#include "CollisionChunk.h"
#include "../World/Coord.h"
#include "../World/SpecializedChunkManager.h"

class VoxelChunk;
class VoxelChunkManager;

// Handles the lifetimes of collision chunks.
class CollisionChunkManager final : public SpecializedChunkManager<CollisionChunk>
{
private:
	// @todo: dynamic collision meshes for entities (stored globally here, not per-chunk)

	void populateChunk(int index, const ChunkInt2 &chunkPos, const VoxelChunk &voxelChunk);
	void updateDirtyVoxels(const ChunkInt2 &chunkPos, const VoxelChunk &voxelChunk);
public:
	void update(double dt, BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
		BufferView<const ChunkInt2> freedChunkPositions, const VoxelChunkManager &voxelChunkManager);
};

#endif
