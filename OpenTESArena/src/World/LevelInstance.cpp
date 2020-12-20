#include "LevelInstance.h"
#include "MapDefinition.h"
#include "WorldType.h"

#include "components/debug/Debug.h"

void LevelInstance::init()
{
	// @todo: remove fixed-size grid dependency in entity manager.
	DebugNotImplemented();
	//this->entityManager.init(-1, -1);
}

ChunkManager &LevelInstance::getChunkManager()
{
	return this->chunkManager;
}

const ChunkManager &LevelInstance::getChunkManager() const
{
	return this->chunkManager;
}

EntityManager &LevelInstance::getEntityManager()
{
	return this->entityManager;
}

const EntityManager &LevelInstance::getEntityManager() const
{
	return this->entityManager;
}

void LevelInstance::update(double dt, const ChunkInt2 &centerChunk, const MapDefinition &mapDefinition,
	int chunkDistance)
{
	this->chunkManager.update(centerChunk, mapDefinition.getWorldType(), chunkDistance,
		this->entityManager);
}
