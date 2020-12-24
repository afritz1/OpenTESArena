#include "LevelInstance.h"
#include "MapDefinition.h"
#include "MapType.h"

#include "components/debug/Debug.h"

LevelInstance::LevelInstance()
{
	this->ceilingScale = 0.0;
}

void LevelInstance::init(double ceilingScale)
{
	this->ceilingScale = ceilingScale;

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

void LevelInstance::update(double dt, const ChunkInt2 &centerChunk,
	int activeLevelIndex, const MapDefinition &mapDefinition, int chunkDistance)
{
	this->chunkManager.update(dt, centerChunk, activeLevelIndex, mapDefinition, chunkDistance,
		this->entityManager);
}
