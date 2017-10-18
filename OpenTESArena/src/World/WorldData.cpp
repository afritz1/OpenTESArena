#include <cassert>

#include "WorldData.h"

#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"

WorldData::WorldData(const MIFFile &mif, const INFFile &inf)
{
	// Generate levels.
	for (const auto &level : mif.getLevels())
	{
		this->levels.push_back(LevelData(level, inf, mif.getWidth(), mif.getDepth()));
	}

	// Convert start points from the old coordinate system to the new one.
	for (const auto &point : mif.getStartPoints())
	{
		this->startPoints.push_back(VoxelGrid::arenaVoxelToNewVoxel(
			point, mif.getWidth(), mif.getDepth()));
	}

	this->currentLevel = mif.getStartingLevelIndex();
}

WorldData::WorldData(VoxelGrid &&voxelGrid, EntityManager &&entityManager)
	: entityManager(std::move(entityManager))
{
	this->levels.push_back(LevelData(std::move(voxelGrid)));
	this->currentLevel = 0;
}

WorldData::~WorldData()
{

}

int WorldData::getCurrentLevel() const
{
	return this->currentLevel;
}

EntityManager &WorldData::getEntityManager()
{
	return this->entityManager;
}

const EntityManager &WorldData::getEntityManager() const
{
	return this->entityManager;
}

const std::vector<Double2> &WorldData::getStartPoints() const
{
	return this->startPoints;
}

std::vector<LevelData> &WorldData::getLevels()
{
	return this->levels;
}

const std::vector<LevelData> &WorldData::getLevels() const
{
	return this->levels;
}

void WorldData::switchToLevel(int levelIndex, TextureManager &textureManager,
	Renderer &renderer)
{
	assert(levelIndex < this->levels.size());
	this->currentLevel = levelIndex;

	// To do: refresh world state, textures, etc..
}
