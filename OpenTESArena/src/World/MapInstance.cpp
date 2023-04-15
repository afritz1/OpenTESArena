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

void MapInstance::initInterior(const MapDefinition &mapDefinition, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssert(mapDefinition.getSubDefinition().type == MapType::Interior);

	const BufferView<const LevelDefinition> levelDefs = mapDefinition.getLevels();
	const int levelCount = levelDefs.getCount();
	this->levels.init(levelCount);
	this->skies.init(levelCount);

	for (int i = 0; i < levelCount; i++)
	{
		// Initialize level instance.
		const BufferView<const int> levelInfoDefIndices = mapDefinition.getLevelInfoIndices();
		const BufferView<const LevelInfoDefinition> levelInfoDefs = mapDefinition.getLevelInfos();
		const int levelInfoDefIndex = levelInfoDefIndices[i];
		const LevelInfoDefinition &levelInfoDefinition = levelInfoDefs[levelInfoDefIndex];
		LevelInstance &levelInst = this->levels.get(i);
		levelInst.init(levelInfoDefinition.getCeilingScale());
		
		// Initialize sky instance.
		const int skyIndex = mapDefinition.getSkyIndexForLevel(i);
		const SkyDefinition &skyDefinition = mapDefinition.getSky(skyIndex);
		const SkyInfoDefinition &skyInfoDefinition = mapDefinition.getSkyInfoForSky(skyIndex);
		constexpr int currentDay = 0; // Doesn't matter for interiors.
		SkyInstance &skyInst = this->skies.get(i);
		skyInst.init(skyDefinition, skyInfoDefinition, currentDay, textureManager, renderer);
	}

	// Set active level/sky.
	const std::optional<int> &startLevelIndex = mapDefinition.getStartLevelIndex();	
	DebugAssert(startLevelIndex.has_value());
	this->activeLevelIndex = *startLevelIndex;
	this->activeSkyIndex = mapDefinition.getSkyIndexForLevel(this->activeLevelIndex);
}

void MapInstance::initCity(const MapDefinition &mapDefinition, int currentDay, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssert(mapDefinition.getSubDefinition().type == MapType::City);
	this->levels.init(1);
	this->skies.init(1);

	// Initialize level instance for the city.
	const BufferView<const LevelInfoDefinition> levelInfoDefs = mapDefinition.getLevelInfos();
	const LevelInfoDefinition &levelInfoDefinition = levelInfoDefs[0];
	LevelInstance &levelInst = this->levels.get(0);
	levelInst.init(levelInfoDefinition.getCeilingScale());

	// Initialize sky instance.
	const SkyDefinition &skyDefinition = mapDefinition.getSky(0);
	const SkyInfoDefinition &skyInfoDefinition = mapDefinition.getSkyInfoForSky(0);
	SkyInstance &skyInst = this->skies.get(0);
	skyInst.init(skyDefinition, skyInfoDefinition, currentDay, textureManager, renderer);

	// Set active level/sky.
	const std::optional<int> &startLevelIndex = mapDefinition.getStartLevelIndex();
	DebugAssert(startLevelIndex.has_value() && (*startLevelIndex == 0));
	this->activeLevelIndex = 0;
	this->activeSkyIndex = 0;
}

void MapInstance::initWild(const MapDefinition &mapDefinition, int currentDay, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssert(mapDefinition.getSubDefinition().type == MapType::Wilderness);
	this->levels.init(1);
	this->skies.init(1);

	// Initialize level instance for the wild.
	const BufferView<const LevelInfoDefinition> levelInfoDefs = mapDefinition.getLevelInfos();
	const LevelInfoDefinition &levelInfoDefinition = levelInfoDefs[0];
	LevelInstance &levelInst = this->levels.get(0);
	levelInst.init(levelInfoDefinition.getCeilingScale());

	// Initialize sky instance.
	const SkyDefinition &skyDefinition = mapDefinition.getSky(0);
	const SkyInfoDefinition &skyInfoDefinition = mapDefinition.getSkyInfoForSky(0);
	SkyInstance &skyInst = this->skies.get(0);
	skyInst.init(skyDefinition, skyInfoDefinition, currentDay, textureManager, renderer);

	// Set active level/sky.
	const std::optional<int> &startLevelIndex = mapDefinition.getStartLevelIndex();
	DebugAssert(!startLevelIndex.has_value());
	this->activeLevelIndex = 0;
	this->activeSkyIndex = 0;
}

void MapInstance::init(const MapDefinition &mapDefinition, int currentDay, TextureManager &textureManager, Renderer &renderer)
{
	const MapType mapType = mapDefinition.getSubDefinition().type;
	if (mapType == MapType::Interior)
	{
		this->initInterior(mapDefinition, textureManager, renderer);
	}
	else if (mapType == MapType::City)
	{
		this->initCity(mapDefinition, currentDay, textureManager, renderer);
	}
	else if (mapType == MapType::Wilderness)
	{
		this->initWild(mapDefinition, currentDay, textureManager, renderer);
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

void MapInstance::setActiveLevelIndex(int levelIndex, const MapDefinition &mapDefinition)
{
	DebugAssert(levelIndex >= 0);
	DebugAssert(levelIndex < this->levels.getCount());
	this->activeLevelIndex = levelIndex;	
	this->activeSkyIndex = mapDefinition.getSkyIndexForLevel(levelIndex);
}

void MapInstance::update(double dt, Game &game, const MapDefinition &mapDefinition, double latitude, double daytimePercent,
	const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, AudioManager &audioManager)
{
	LevelInstance &levelInst = this->getActiveLevel();
	const ChunkManager &chunkManager = game.getChunkManager();
	const BufferView<const ChunkInt2> activeChunkPositions = chunkManager.getActiveChunkPositions();
	const BufferView<const ChunkInt2> newChunkPositions = chunkManager.getNewChunkPositions();
	const BufferView<const ChunkInt2> freedChunkPositions = chunkManager.getFreedChunkPositions();
	RenderChunkManager &renderChunkManager = game.getRenderChunkManager();
	const GameState &gameState = game.getGameState();
	const double chasmAnimPercent = gameState.getChasmAnimPercent();
	levelInst.update(dt, activeChunkPositions, newChunkPositions, freedChunkPositions, game.getPlayer(),
		this->activeLevelIndex, mapDefinition, entityGenInfo, citizenGenInfo, chasmAnimPercent, game.getRandom(),
		entityDefLibrary, binaryAssetLibrary, renderChunkManager, textureManager, audioManager, game.getRenderer());

	SkyInstance &skyInst = this->getActiveSky();
	const WeatherInstance &weatherInst = game.getGameState().getWeatherInstance();
	skyInst.update(dt, latitude, daytimePercent, weatherInst, game.getRandom(), textureManager);
}

void MapInstance::cleanUp()
{
	LevelInstance &levelInst = this->getActiveLevel();
	levelInst.cleanUp();
}
