#ifndef COLLISION_CHUNK_MANAGER_H
#define COLLISION_CHUNK_MANAGER_H

#include <memory>
#include <optional>
#include <vector>

#include "CollisionChunk.h"
#include "../World/Coord.h"
#include "../World/SpecializedChunkManager.h"

// Handles the lifetimes of collision chunks.
class CollisionChunkManager final : SpecializedChunkManager<CollisionChunk>
{
private:
	// @todo: dynamic collision meshes for entities (stored globally here, not per-chunk)
public:
	void update(double dt, const BufferView<const ChunkInt2> &newChunkPositions,
		const BufferView<const ChunkInt2> &freedChunkPositions);
};

#endif
