#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include <memory>
#include <vector>

#include "Chunk.h"
#include "ChunkUtils.h"
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
	using ChunkPtr = std::unique_ptr<Chunk>;

	std::vector<ChunkPtr> chunkPool;
	std::vector<ChunkPtr> activeChunks;
	WorldType worldType;
	int chunkDistance;

	// Returns whether the given chunk ID points to an active chunk. The chunk ID of a chunk can
	// never change until it has returned to the chunk pool.
	bool isValidChunkID(ChunkID id) const;

	// Gets a pointer to the chunk associated with the ID if it is active.
	ChunkPtr &getChunkPtr(ChunkID id);
	const ChunkPtr &getChunkPtr(ChunkID id) const;

	// Takes a chunk from the chunk pool and moves it to the active chunks.
	ChunkID spawnChunk();

	// Clears the chunk, including entities, and removes it from the active chunks.
	void recycleChunk(ChunkID id, EntityManager &entityManager);

	// Fills the chunk with the data required based on its position and the world type.
	bool populateChunk(ChunkID id, WorldType worldType, EntityManager &entityManager);
public:
	ChunkManager();

	void init(WorldType worldType, int chunkDistance);

	int getChunkCount() const;
	ChunkID getChunkID(int index) const;
	bool tryGetChunkID(const ChunkInt2 &coord, ChunkID *outID) const;
	Chunk &getChunk(ChunkID id);
	const Chunk &getChunk(ChunkID id) const;

	// Updates the chunk manager with the given chunk as the current center of the game world.
	// This invalidates all existing chunk IDs.
	void update(const ChunkInt2 &playerChunk, WorldType worldType, EntityManager &entityManager);
};

#endif
