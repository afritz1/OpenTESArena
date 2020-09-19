#include "LevelInstance.h"
#include "WorldType.h"

#include "components/debug/Debug.h"

LevelInstance::LevelInstance()
{
	this->worldType = static_cast<WorldType>(-1);
}

void LevelInstance::init(WorldType worldType, int chunkDistance)
{
	this->worldType = worldType;
	this->chunkManager.init(chunkDistance);

	// @todo: remove fixed-size grid dependency in entity manager.
	DebugNotImplemented();
	//this->entityManager.init(-1, -1);
}

void LevelInstance::initInterior(int chunkDistance)
{
	this->init(WorldType::Interior, chunkDistance);
}

void LevelInstance::initCity(int chunkDistance)
{
	this->init(WorldType::City, chunkDistance);
}

void LevelInstance::initWilderness(int chunkDistance)
{
	this->init(WorldType::Wilderness, chunkDistance);
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

void LevelInstance::update(double dt)
{
	// @todo: call update on chunk manager so it can remove finished voxel instances, etc..
	// See LevelData::updateFadingVoxels() for reference.

	DebugNotImplemented();
}
