#include "MapDefinition.h"
#include "MapInstance.h"
#include "MapType.h"

#include "components/debug/Debug.h"

MapInstance::MapInstance()
{
	this->activeLevelIndex = -1;
	this->activeSkyIndex = -1;
}

void MapInstance::initInterior(const MapDefinition &mapDefinition, TextureManager &textureManager)
{
	DebugAssert(mapDefinition.getMapType() == MapType::Interior);
	this->levels.init(mapDefinition.getLevelCount());
	this->skies.init(this->levels.getCount());

	for (int i = 0; i < this->levels.getCount(); i++)
	{
		// Initialize level instance.
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(i);
		LevelInstance &levelInst = this->levels.get(i);
		levelInst.init(levelInfoDefinition.getCeilingScale());
		
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

void MapInstance::initCity(const MapDefinition &mapDefinition, TextureManager &textureManager)
{
	DebugAssert(mapDefinition.getMapType() == MapType::City);
	this->levels.init(1);
	this->skies.init(1);

	// Initialize level instance for the city.
	const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(0);
	LevelInstance &levelInst = this->levels.get(0);
	levelInst.init(levelInfoDefinition.getCeilingScale());

	// Initialize sky instance.
	const SkyDefinition &skyDefinition = mapDefinition.getSky(0);
	const SkyInfoDefinition &skyInfoDefinition = mapDefinition.getSkyInfoForSky(0);
	SkyInstance &skyInst = this->skies.get(0);
	skyInst.init(skyDefinition, skyInfoDefinition, textureManager);

	// Set active level/sky.
	const std::optional<int> &startLevelIndex = mapDefinition.getStartLevelIndex();
	DebugAssert(startLevelIndex.has_value() && (*startLevelIndex == 0));
	this->activeLevelIndex = 0;
	this->activeSkyIndex = 0;
}

void MapInstance::initWild(const MapDefinition &mapDefinition, TextureManager &textureManager)
{
	DebugAssert(mapDefinition.getMapType() == MapType::Wilderness);
	this->levels.init(1);
	this->skies.init(1);

	// Initialize level instance for the wild.
	const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(0);
	LevelInstance &levelInst = this->levels.get(0);
	levelInst.init(levelInfoDefinition.getCeilingScale());

	// Initialize sky instance.
	const SkyDefinition &skyDefinition = mapDefinition.getSky(0);
	const SkyInfoDefinition &skyInfoDefinition = mapDefinition.getSkyInfoForSky(0);
	SkyInstance &skyInst = this->skies.get(0);
	skyInst.init(skyDefinition, skyInfoDefinition, textureManager);

	// Set active level/sky.
	const std::optional<int> &startLevelIndex = mapDefinition.getStartLevelIndex();
	DebugAssert(!startLevelIndex.has_value());
	this->activeLevelIndex = 0;
	this->activeSkyIndex = 0;
}

void MapInstance::init(const MapDefinition &mapDefinition, TextureManager &textureManager)
{
	const MapType mapType = mapDefinition.getMapType();
	if (mapType == MapType::Interior)
	{
		this->initInterior(mapDefinition, textureManager);
	}
	else if (mapType == MapType::City)
	{
		this->initCity(mapDefinition, textureManager);
	}
	else if (mapType == MapType::Wilderness)
	{
		this->initWild(mapDefinition, textureManager);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mapType)));
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
	
	// @todo: update sky level index
	DebugNotImplemented();
}

void MapInstance::update(double dt, const ChunkInt2 &centerChunk,
	const MapDefinition &mapDefinition, double latitude, double daytimePercent, int chunkDistance)
{
	LevelInstance &levelInst = this->getActiveLevel();
	levelInst.update(dt, centerChunk, this->activeLevelIndex, mapDefinition, chunkDistance);

	SkyInstance &skyInst = this->getActiveSky();
	skyInst.update(dt, latitude, daytimePercent);
}
