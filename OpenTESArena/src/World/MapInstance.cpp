#include "MapDefinition.h"
#include "MapInstance.h"
#include "WorldType.h"

#include "components/debug/Debug.h"

MapInstance::MapInstance()
{
	this->activeLevelIndex = -1;
	this->activeSkyIndex = -1;
}

void MapInstance::initInterior(const MapDefinition &mapDefinition, const TextureManager &textureManager)
{
	DebugAssert(mapDefinition.getWorldType() == WorldType::Interior);
	this->levels.init(mapDefinition.getLevelCount());
	this->skies.init(this->levels.getCount());

	for (int i = 0; i < this->levels.getCount(); i++)
	{
		// Initialize level instance.
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(i);
		LevelInstance &levelInst = this->levels.get(i);
		levelInst.init();
		
		// Initialize sky instance.
		const int skyIndex = mapDefinition.getSkyIndexForLevel(i);
		const SkyDefinition &skyDefinition = mapDefinition.getSky(skyIndex);
		const SkyInfoDefinition &skyInfoDefinition = mapDefinition.getSkyInfoForSky(skyIndex);
		SkyInstance &skyInst = this->skies.get(i);
		skyInst.init(skyDefinition, skyInfoDefinition, textureManager);
	}

	// Set active level/sky.
	const std::optional<int> &startLevelIndex = mapDefinition.getStartLevelIndex();	
	DebugAssert(startLevelIndex.has_value());
	this->activeLevelIndex = *startLevelIndex;
	this->activeSkyIndex = mapDefinition.getSkyIndexForLevel(this->activeLevelIndex);
}

void MapInstance::initCity(const MapDefinition &mapDefinition, const TextureManager &textureManager)
{
	DebugAssert(mapDefinition.getWorldType() == WorldType::City);
	this->levels.init(1);
	this->skies.init(1);

	DebugNotImplemented();
}

void MapInstance::initWild(const MapDefinition &mapDefinition, const TextureManager &textureManager)
{
	DebugAssert(mapDefinition.getWorldType() == WorldType::Wilderness);
	this->levels.init(1);
	this->skies.init(1);

	DebugNotImplemented();
}

void MapInstance::init(const MapDefinition &mapDefinition, const TextureManager &textureManager)
{
	const WorldType worldType = mapDefinition.getWorldType();
	if (worldType == WorldType::Interior)
	{
		this->initInterior(mapDefinition, textureManager);
	}
	else if (worldType == WorldType::City)
	{
		this->initCity(mapDefinition, textureManager);
	}
	else if (worldType == WorldType::Wilderness)
	{
		this->initWild(mapDefinition, textureManager);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(worldType)));
	}
}

int MapInstance::getLevelCount() const
{
	return this->levels.getCount();
}

LevelInstance &MapInstance::getLevel(int index)
{
	return this->levels.get(index);
}

const LevelInstance &MapInstance::getLevel(int index) const
{
	return this->levels.get(index);
}

LevelInstance &MapInstance::getActiveLevel()
{
	return this->levels.get(this->activeLevelIndex);
}

const LevelInstance &MapInstance::getActiveLevel() const
{
	return this->levels.get(this->activeLevelIndex);
}

int MapInstance::getSkyCount() const
{
	return this->skies.getCount();
}

SkyInstance &MapInstance::getSky(int index)
{
	return this->skies.get(index);
}

const SkyInstance &MapInstance::getSky(int index) const
{
	return this->skies.get(index);
}

SkyInstance &MapInstance::getActiveSky()
{
	return this->skies.get(this->activeSkyIndex);
}

const SkyInstance &MapInstance::getActiveSky() const
{
	return this->skies.get(this->activeSkyIndex);
}

void MapInstance::setActiveLevelIndex(int levelIndex)
{
	DebugAssert(levelIndex >= 0);
	DebugAssert(levelIndex < this->levels.getCount());
	this->activeLevelIndex = levelIndex;
}
