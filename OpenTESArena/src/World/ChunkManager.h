#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include <memory>
#include <vector>

#include "Chunk.h"
#include "VoxelUtils.h"

// Handles active chunks and the voxels in them. Does not store any entities. When freeing a chunk,
// it needs to tell the entity manager about it so the entities in it are handled correctly
// (i.e. marked for deletion one way or another).

class EntityManager;
class Game;

enum class WorldType;

class ChunkManager
{
private:
	std::vector<std::unique_ptr<Chunk>> chunkPool;
	std::vector<std::unique_ptr<Chunk>> activeChunks;

	int findChunkIndex(const ChunkInt2 &coord) const;
public:
	void init(int chunkDistance);

	int getChunkCount() const;
	Chunk &getChunkAtIndex(int index);
	const Chunk &getChunkAtIndex(int index) const;
	Chunk *getChunk(const ChunkInt2 &coord);
	const Chunk *getChunk(const ChunkInt2 &coord) const;

	// Fills the chunk with the data required based on its position and the world type.
	bool tryPopulateChunk(const ChunkInt2 &coord, WorldType worldType, Game &game);

	// Clears the chunk and removes it from the active chunks.
	bool tryFreeChunk(const ChunkInt2 &coord, EntityManager &entityManager);

	// Clears all voxels and entities in the active chunks.
	void clear(EntityManager &entityManager);
};

#endif
