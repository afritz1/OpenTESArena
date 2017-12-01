#include <cassert>

#include "WorldData.h"
#include "WorldType.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"

WorldData::WorldData(const MIFFile &mif, const INFFile &inf, WorldType type)
{
	// Generate levels.
	for (const auto &level : mif.getLevels())
	{
		// Not sure yet how to differentiate a location between "interior" and "exterior".
		// All interiors have ceilings except some main quest dungeons which have a 1
		// as the third number after *CEILING in their .INF file.
		const bool isInterior = [&inf, type, &level]()
		{
			if (type == WorldType::City)
			{
				return false;
			}
			else if (type == WorldType::Interior)
			{
				return !inf.getCeiling().outdoorDungeon;
			}
			else
			{
				return false;
			}
		}();

		this->levels.push_back(LevelData(level, inf, 
			mif.getWidth(), mif.getDepth(), isInterior));
	}

	// Convert start points from the old coordinate system to the new one.
	for (const auto &point : mif.getStartPoints())
	{
		this->startPoints.push_back(VoxelGrid::arenaVoxelToNewVoxel(
			point, mif.getWidth(), mif.getDepth()));
	}

	this->currentLevel = mif.getStartingLevelIndex();
	this->worldType = worldType;
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

WorldType WorldData::getWorldType() const
{
	return this->worldType;
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
