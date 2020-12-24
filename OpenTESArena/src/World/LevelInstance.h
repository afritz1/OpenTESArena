#ifndef LEVEL_INSTANCE_H
#define LEVEL_INSTANCE_H

#include <optional>

#include "ChunkManager.h"
#include "../Entities/EntityManager.h"

// Instance of a level with voxels and entities. Its data is in a baked, context-sensitive format
// and depends on one or more level definitions for its population.

class MapDefinition;

enum class MapType;

class LevelInstance
{
private:
	ChunkManager chunkManager;
	EntityManager entityManager;
	double ceilingScale;
public:
	LevelInstance();

	void init(double ceilingScale);

	ChunkManager &getChunkManager();
	const ChunkManager &getChunkManager() const;
	EntityManager &getEntityManager();
	const EntityManager &getEntityManager() const;

	void update(double dt, const ChunkInt2 &centerChunk, int activeLevelIndex,
		const MapDefinition &mapDefinition, int chunkDistance);

	// @todo: some "setActive()" like LevelData so the renderer can be initialized with this level's data.
	// Probably also store the table of asset filenames/ImageIDs/etc. -> voxel/entity/etc. texture IDs in
	// this class.
};

#endif
