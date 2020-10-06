#ifndef LEVEL_INSTANCE_H
#define LEVEL_INSTANCE_H

#include "ChunkManager.h"
#include "../Entities/EntityManager.h"

// Instance of a level with voxels and entities. Its data is in a baked, context-sensitive format
// and depends on one or more level definitions for its population.

enum class WorldType;

class LevelInstance
{
private:
	ChunkManager chunkManager;
	EntityManager entityManager;
	WorldType worldType;

	void init(WorldType worldType, int chunkDistance); // @todo: give WorldType?
	// @todo: chunkDistance will need to be a set() function in ChunkManager so it can change the chunk pool at runtime after init.
public:
	LevelInstance();

	void initInterior(int chunkDistance);
	void initCity(int chunkDistance);
	void initWilderness(int chunkDistance);

	ChunkManager &getChunkManager();
	const ChunkManager &getChunkManager() const;

	EntityManager &getEntityManager();
	const EntityManager &getEntityManager() const;

	void update(double dt);
};

#endif
