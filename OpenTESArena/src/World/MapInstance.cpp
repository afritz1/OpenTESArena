#include "ArenaCityUtils.h"
#include "ArenaWildUtils.h"
#include "MapDefinition.h"
#include "MapInstance.h"
#include "MapType.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"

MapInstance::MapInstance()
{
	this->activeLevelIndex = -1;
	this->activeSkyIndex = -1;
}

bool MapInstance::initInterior(const MapDefinition &mapDefinition, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssert(mapDefinition.getMapType() == MapType::Interior);
	const MapDefinition::Interior &mapDefInterior = mapDefinition.getInterior();
	const bool isProcedural = mapDefInterior.isProcedural;

	const int levelCount = mapDefinition.getLevelCount();
	const int levelInfoCount = isProcedural ? 1 : levelCount; // In dungeons, all levels point to the first level info.
	this->levels.init(levelCount);
	this->levelInfos.init(levelInfoCount);
	this->levelInfoMappings.init(levelCount);
	this->skies.init(levelCount);
	this->skyInfos.init(levelCount);
	this->skyInfoMappings.init(levelCount);

	for (int i = 0; i < levelInfoCount; i++)
	{
		const std::vector<std::string> &infNames = mapDefInterior.infNames;
		DebugAssertIndex(infNames, i);
		const std::string infName = infNames[i];
		INFFile inf;
		if (!inf.init(infName.c_str()))
		{
			DebugLogError("Couldn't init .INF file \"" + infName + "\".");
			return false;
		}

		const INFFile::CeilingData &ceiling = inf.getCeiling();
		const double ceilingScale = ArenaLevelUtils::convertCeilingHeightToScale(ceiling.height);

		LevelInfoDefinition &levelInfoDef = this->levelInfos.get(i);
		levelInfoDef.init(ceilingScale);

		if (isProcedural)
		{
			MapGeneration::generateMifDungeonInfo(inf, &levelInfoDef);
		}
		else
		{
			MapGeneration::generateMifInteriorInfo(inf, &levelInfoDef);
		}
	}

	for (int i = 0; i < levelCount; i++)
	{
		SkyInfoDefinition &skyInfoDef = this->skyInfos.get(i);
		SkyGeneration::generateInteriorSkyInfo(&skyInfoDef);

		const int levelInfoMappingIndex = isProcedural ? 0 : i;
		this->levelInfoMappings.set(i, levelInfoMappingIndex);
		this->skyInfoMappings.set(i, i);

		const LevelInfoDefinition &levelInfoDef = this->getLevelInfoForLevel(levelInfoMappingIndex);

		// Initialize level instance.
		LevelInstance &levelInst = this->levels.get(i);
		levelInst.init(levelInfoDef.getCeilingScale());
		
		// Initialize sky instance.
		const int skyIndex = mapDefinition.getSkyIndexForLevel(i);
		const SkyDefinition &skyDefinition = mapDefinition.getSky(skyIndex);
		const SkyInfoDefinition &skyInfoDefinition = this->getSkyInfoForSky(skyIndex);
		const int allowedWeatherDefIndex = skyDefinition.getAllowedWeatherIndex(ArenaTypes::WeatherType::Clear); // Assume clear for interiors.
		constexpr int currentDay = 0; // Doesn't matter for interiors.
		SkyInstance &skyInst = this->skies.get(i);
		skyInst.init(skyDefinition, skyInfoDefinition, allowedWeatherDefIndex, currentDay, textureManager, renderer);
	}

	// Set active level + sky.
	const std::optional<int> &startLevelIndex = mapDefinition.getStartLevelIndex();	
	DebugAssert(startLevelIndex.has_value());
	this->activeLevelIndex = *startLevelIndex;
	this->activeSkyIndex = mapDefinition.getSkyIndexForLevel(this->activeLevelIndex);

	return true;
}

bool MapInstance::initCity(const MapDefinition &mapDefinition, ArenaTypes::ClimateType climateType, ArenaTypes::WeatherType weatherType,
	int currentDay, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssert(mapDefinition.getMapType() == MapType::City);
	
	// 1 LevelDefinition and 1 LevelInfoDefinition.
	this->levels.init(1);
	this->levelInfos.init(1);
	this->levelInfoMappings.init(1);
	this->skies.init(1);
	this->skyInfos.init(1);
	this->skyInfoMappings.init(1);

	const std::string infName = ArenaCityUtils::generateInfName(climateType, weatherType);
	INFFile inf;
	if (!inf.init(infName.c_str()))
	{
		DebugLogError("Couldn't init city .INF file \"" + infName + "\".");
		return false;
	}

	const INFFile::CeilingData &ceiling = inf.getCeiling();
	const double ceilingScale = ArenaLevelUtils::convertCeilingHeightToScale(ceiling.height);

	LevelInfoDefinition &levelInfoDef = this->levelInfos.get(0);
	levelInfoDef.init(ceilingScale);
	MapGeneration::generateMifCityInfo(inf, &levelInfoDef);

	SkyInfoDefinition &skyInfoDef = this->skyInfos.get(0);
	SkyGeneration::generateExteriorSkyInfo(&skyInfoDef);

	// Only one level info and sky to use.
	this->levelInfoMappings.set(0, 0);
	this->skyInfoMappings.set(0, 0);

	// Initialize level instance for the city.
	LevelInstance &levelInst = this->levels.get(0);
	levelInst.init(ceilingScale);

	// Initialize sky instance.
	const SkyDefinition &skyDefinition = mapDefinition.getSky(0);
	const SkyInfoDefinition &skyInfoDefinition = this->skyInfos.get(0);
	const int allowedWeatherDefIndex = skyDefinition.getAllowedWeatherIndex(weatherType);
	SkyInstance &skyInst = this->skies.get(0);
	skyInst.init(skyDefinition, skyInfoDefinition, allowedWeatherDefIndex, currentDay, textureManager, renderer);

	// Set active level + sky.
	const std::optional<int> &startLevelIndex = mapDefinition.getStartLevelIndex();
	DebugAssert(startLevelIndex.has_value() && (*startLevelIndex == 0));
	this->activeLevelIndex = 0;
	this->activeSkyIndex = 0;
}

bool MapInstance::initWild(const MapDefinition &mapDefinition, ArenaTypes::ClimateType climateType, ArenaTypes::WeatherType weatherType,
	int currentDay, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssert(mapDefinition.getMapType() == MapType::Wilderness);

	// Every wild chunk level definition uses the same level info definition.
	this->levels.init(1);
	this->levelInfos.init(1);
	this->levelInfoMappings.init(mapDefinition.getLevelCount());
	this->skies.init(1);
	this->skyInfos.init(1);
	this->skyInfoMappings.init(1);

	const std::string infName = ArenaWildUtils::generateInfName(climateType, weatherType);
	INFFile inf;
	if (!inf.init(infName.c_str()))
	{
		DebugLogError("Couldn't init wild .INF file \"" + infName + "\".");
		return false;
	}

	const INFFile::CeilingData &ceiling = inf.getCeiling();
	const double ceilingScale = ArenaLevelUtils::convertCeilingHeightToScale(ceiling.height);

	LevelInfoDefinition &levelInfoDef = this->levelInfos.get(0);
	levelInfoDef.init(ceilingScale);
	MapGeneration::generateRmdWildernessInfo(inf, &levelInfoDef);

	SkyInfoDefinition &skyInfoDef = this->skyInfos.get(0);
	SkyGeneration::generateExteriorSkyInfo(&skyInfoDef);
	
	for (int i = 0; i < this->levelInfoMappings.getCount(); i++)
	{
		this->levelInfoMappings.set(i, 0);
	}

	this->skyInfoMappings.set(0, 0);

	// Initialize level instance for the wild.
	LevelInstance &levelInst = this->levels.get(0);
	levelInst.init(ceilingScale);

	// Initialize sky instance.
	const SkyDefinition &skyDefinition = mapDefinition.getSky(0);
	const SkyInfoDefinition &skyInfoDefinition = this->skyInfos.get(0);
	const int allowedWeatherDefIndex = skyDefinition.getAllowedWeatherIndex(weatherType);
	SkyInstance &skyInst = this->skies.get(0);
	skyInst.init(skyDefinition, skyInfoDefinition, allowedWeatherDefIndex, currentDay, textureManager, renderer);

	// Set active level + sky.
	const std::optional<int> &startLevelIndex = mapDefinition.getStartLevelIndex();
	DebugAssert(!startLevelIndex.has_value());
	this->activeLevelIndex = 0;
	this->activeSkyIndex = 0;
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

int MapInstance::getActiveLevelIndex() const
{
	return this->activeLevelIndex;
}

LevelInstance &MapInstance::getActiveLevel()
{
	return this->levels.get(this->activeLevelIndex);
}

const LevelInstance &MapInstance::getActiveLevel() const
{
	return this->levels.get(this->activeLevelIndex);
}

const LevelInfoDefinition &MapInstance::getLevelInfoForLevel(int levelIndex) const
{
	const int levelInfoIndex = this->levelInfoMappings.get(levelIndex);
	return this->levelInfos.get(levelInfoIndex);
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

const SkyInfoDefinition &MapInstance::getSkyInfoForSky(int skyIndex) const
{
	const int skyInfoIndex = this->skyInfoMappings.get(skyIndex);
	return this->skyInfos.get(skyInfoIndex);
}

void MapInstance::setActiveLevelIndex(int levelIndex, const MapDefinition &mapDefinition)
{
	DebugAssert(levelIndex >= 0);
	DebugAssert(levelIndex < this->levels.getCount());
	this->activeLevelIndex = levelIndex;	
	this->activeSkyIndex = mapDefinition.getSkyIndexForLevel(levelIndex);
}

void MapInstance::update(double dt, Game &game, const CoordDouble3 &playerCoord, const MapDefinition &mapDefinition,
	double latitude, double daytimePercent, const EntityGeneration::EntityGenInfo &entityGenInfo,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, AudioManager &audioManager)
{
	LevelInstance &levelInst = this->getActiveLevel();
	const ChunkManager &chunkManager = game.getChunkManager();
	const BufferView<const ChunkInt2> activeChunkPositions = chunkManager.getActiveChunkPositions();
	const BufferView<const ChunkInt2> newChunkPositions = chunkManager.getNewChunkPositions();
	const BufferView<const ChunkInt2> freedChunkPositions = chunkManager.getFreedChunkPositions();
	RenderChunkManager &renderChunkManager = game.getRenderChunkManager();
	const GameState &gameState = game.getGameState();
	const double chasmAnimPercent = gameState.getChasmAnimPercent();
	levelInst.update(dt, activeChunkPositions, newChunkPositions, freedChunkPositions, playerCoord, this->activeLevelIndex,
		mapDefinition, *this, entityGenInfo, citizenGenInfo, chasmAnimPercent, game.getRandom(), entityDefLibrary,
		binaryAssetLibrary, renderChunkManager, textureManager, audioManager, game.getRenderer());

	SkyInstance &skyInst = this->getActiveSky();
	const WeatherInstance &weatherInst = game.getGameState().getWeatherInstance();
	skyInst.update(dt, latitude, daytimePercent, weatherInst, game.getRandom(), textureManager);
}

void MapInstance::cleanUp()
{
	LevelInstance &levelInst = this->getActiveLevel();
	levelInst.cleanUp();
}
