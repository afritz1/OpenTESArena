#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include <memory>
#include <optional>
#include <vector>

#include "Chunk.h"
#include "ChunkUtils.h"
#include "VoxelUtils.h"

// Handles lifetimes of chunks. Does not store any entities. When freeing a chunk, it needs to tell
// the entity manager so the entities in it are handled correctly (marked for deletion one way or
// another).

class EntityManager;
class Game;
class LevelDefinition;
class LevelInfoDefinition;
class MapDefinition;

enum class MapType;

class ChunkManager
{
private:
	using ChunkPtr = std::unique_ptr<Chunk>;

	std::vector<ChunkPtr> chunkPool;
	std::vector<ChunkPtr> activeChunks;
	ChunkInt2 centerChunk;

	// Takes a chunk from the chunk pool, moves it to the active chunks, and returns its index.
	int spawnChunk();

	// Clears the chunk, including entities, and removes it from the active chunks.
	void recycleChunk(int index, EntityManager &entityManager);

	// Helper function for setting the chunk's voxels and definitions from the given level. This might
	// not touch all voxels in the chunk because it does not fully overlap the level.
	void populateChunkFromLevel(Chunk &chunk, const LevelDefinition &levelDefinition,
		const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset);

	// Fills the chunk with the data required based on its position and the world type.
	bool populateChunk(int index, const ChunkInt2 &coord, int activeLevelIndex,
		const MapDefinition &mapDefinition);
public:
	int getChunkCount() const;
	Chunk &getChunk(int index);
	const Chunk &getChunk(int index) const;
	std::optional<int> tryGetChunkIndex(const ChunkInt2 &coord) const;

	// Index of the chunk all other active chunks surround.
	int getCenterChunkIndex() const;

	// Updates the chunk manager with the given chunk as the current center of the game world.
	// This invalidates all active chunk references and they must be looked up again.
	void update(double dt, const ChunkInt2 &centerChunk, int activeLevelIndex,
		const MapDefinition &mapDefinition, int chunkDistance, EntityManager &entityManager);
};

#endif
