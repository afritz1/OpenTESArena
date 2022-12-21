#ifndef COLLISION_CHUNK_MANAGER_H
#define COLLISION_CHUNK_MANAGER_H

#include <memory>
#include <optional>
#include <vector>

#include "CollisionChunk.h"
#include "../World/Coord.h"

// Handles the lifetimes of collision chunks.
class CollisionChunkManager
{
private:
	using ChunkPtr = std::unique_ptr<CollisionChunk>;

	std::vector<ChunkPtr> chunkPool;
	std::vector<ChunkPtr> activeChunks;
	// @todo: dynamic collision meshes for entities (stored globally here, not per-chunk)

	std::optional<int> tryGetChunkIndex(const ChunkInt2 &position) const;
	int getChunkIndex(const ChunkInt2 &position) const;
	CollisionChunk &getChunkAtIndex(int index);
	const CollisionChunk &getChunkAtIndex(int index) const;

	// Takes a chunk from the chunk pool, moves it to the active chunks, and returns its index.
	int spawnChunk();

	// Clears the chunk and removes it from the active chunks.
	void recycleChunk(int index);
public:


};

#endif
