#include <algorithm>
#include <array>
#include <cmath>
#include <tuple>

#include "Game.h"
#include "GameState.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaSoundName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/ExeData.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"
#include "../Assets/TextureManager.h"
#include "../Audio/MusicLibrary.h"
#include "../Entities/ArenaEntityUtils.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Interface/GameWorldUiMVC.h"
#include "../Interface/GameWorldUiState.h"
#include "../Interface/LevelUpUiState.h"
#include "../Math/Constants.h"
#include "../Player/Player.h"
#include "../Player/PlayerLogic.h"
#include "../Rendering/RenderCamera.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/RendererUtils.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../Time/ArenaClockUtils.h"
#include "../Time/ClockLibrary.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextRenderUtils.h"
#include "../Voxels/ArenaVoxelUtils.h"
#include "../Weather/ArenaWeatherUtils.h"
#include "../Weather/WeatherUtils.h"
#include "../World/ArenaInteriorUtils.h"
#include "../World/CardinalDirection.h"
#include "../World/MapLogic.h"
#include "../World/MapType.h"
#include "../WorldMap/ArenaLocationUtils.h"
#include "../WorldMap/LocationDefinition.h"
#include "../WorldMap/LocationInstance.h"
#include "../WorldMap/ProvinceLibrary.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	bool CityGuardEntityDefinitionPredicate(const EntityDefinition &entityDef)
	{
		if (entityDef.type != EntityDefinitionType::Enemy)
		{
			return false;
		}

		const EnemyEntityDefinition &enemyDef = entityDef.enemy;
		if (enemyDef.type != EnemyEntityDefinitionType::Human)
		{
			return false;
		}

		const EnemyEntityHumanDefinition &humanEnemyDef = enemyDef.human;
		const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
		const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(humanEnemyDef.charClassID);
		constexpr int originalGuardClassNumber = 16;
		return charClassDef.originalClassIndex == originalGuardClassNumber;
	}

	EntityDefinitionPredicate MakeOriginalSpawnEntityDefinitionPredicate(int spawnID)
	{
		return [spawnID](const EntityDefinition &entityDef)
		{
			if (entityDef.type != EntityDefinitionType::Enemy)
			{
				return false;
			}

			const EnemyEntityDefinition &enemyDef = entityDef.enemy;
			if (spawnID >= ArenaEntityUtils::FinalBossCreatureID)
			{
				if (enemyDef.type != EnemyEntityDefinitionType::Human)
				{
					return false;
				}

				const EnemyEntityHumanDefinition &humanEnemyDef = enemyDef.human;
				const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
				const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(humanEnemyDef.charClassID);
				constexpr int spawnIdToClassNumberDifference = ArenaEntityUtils::FinalBossCreatureID;
				const int originalEnemyClassNumber = spawnID - spawnIdToClassNumberDifference;
				return charClassDef.originalClassIndex == originalEnemyClassNumber;
			}
			else if (enemyDef.type != EnemyEntityDefinitionType::Creature)
			{
				return false;
			}

			const CreatureDefinitionLibrary &creatureDefLibrary = CreatureDefinitionLibrary::getInstance();
			const CreatureDefinitionID spawnCreatureDefID = creatureDefLibrary.getDefinitionIdFromOriginalID(spawnID);
			return enemyDef.creatureDefID == spawnCreatureDefID;
		};
	}
}

GuardSpawnState::GuardSpawnState()
{
	this->clearQueue();
}

bool GuardSpawnState::isQueued() const
{
	return !std::isinf(this->secondsTillSpawn);
}

void GuardSpawnState::clearQueue()
{
	this->secondsTillSpawn = Constants::Infinity;
	this->spawnFunc = []() { };
}

EntityEncounterSpawnInfo::EntityEncounterSpawnInfo()
{
	this->guardType = -1;
	this->level = 0;
	this->count = 0;
}

void EntityEncounterSpawnInfo::initCreaturesOrHumans(int spawnID, int level, int count)
{
	this->guardType = -1;
	this->level = level;
	this->count = count;
	this->entityDefPredicate = MakeOriginalSpawnEntityDefinitionPredicate(spawnID);
}

void EntityEncounterSpawnInfo::initCityGuards(int guardType, int level, int count)
{
	this->guardType = guardType;
	this->level = level;
	this->count = count;
	this->entityDefPredicate = CityGuardEntityDefinitionPredicate;
}

bool EntityEncounterSpawnInfo::isCityGuards() const
{
	return this->guardType >= 0;
}

GameState::WorldMapLocationIDs::WorldMapLocationIDs(int provinceID, int locationID)
{
	this->provinceID = provinceID;
	this->locationID = locationID;
}

GameState::GameState()
{
	DebugLog("Initializing.");

	this->activeLevelIndex = -1;
	this->nextMapClearsPrevious = false;
	this->nextLevelIndex = -1;

	this->isLevelTransitionCalculationPending = false;

	this->clearSession();
}

GameState::~GameState()
{
	DebugLog("Closing.");
}

void GameState::init(ArenaRandom &random)
{
	// @todo: might want a clearSession()? Seems weird.

	// Initialize world map definition and instance to default.
	const ProvinceLibrary &provinceLibrary = ProvinceLibrary::getInstance();
	this->worldMapInst.init(provinceLibrary);

	// @temp: set main quest dungeons visible for testing.
	for (int provinceIndex = 0; provinceIndex < this->worldMapInst.getProvinceCount(); provinceIndex++)
	{
		ProvinceInstance &provinceInst = this->worldMapInst.getProvinceInstance(provinceIndex);
		const ProvinceDefinition &provinceDef = provinceLibrary.getProvinceDef(provinceIndex);

		for (int locationIndex = 0; locationIndex < provinceInst.getLocationCount(); locationIndex++)
		{
			LocationInstance &locationInst = provinceInst.getLocationInstance(locationIndex);
			const int locationDefIndex = locationInst.getLocationDefIndex();
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
			const std::string &locationName = locationInst.getName(locationDef);

			const bool isMainQuestDungeon = locationDef.getType() == LocationDefinitionType::MainQuestDungeon;
			const bool isStartDungeon = isMainQuestDungeon && (locationDef.getMainQuestDungeonDefinition().type == LocationMainQuestDungeonDefinitionType::Start);
			const bool shouldSetVisible = (locationName.size() > 0) && isMainQuestDungeon && !isStartDungeon && !locationInst.isVisible();

			if (shouldSetVisible)
			{
				locationInst.toggleVisibility();
			}
		}
	}

	// Do initial weather update (to set each value to a valid state).
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	this->updateWeatherList(random, exeData);

	this->date = Date();
	this->weatherInst = WeatherInstance();
}

void GameState::clearSession()
{
	// @todo: this function doesn't clear everything, i.e. weather state. Might want to revise later.

	this->isLevelTransitionCalculationPending = false;

	// Don't have to clear on-screen text box durations.
	this->provinceIndex = -1;
	this->locationIndex = -1;

	this->isCamping = false;
	this->chasmAnimSeconds = 0.0;

	this->travelData = std::nullopt;
	this->clearMaps();

	this->weatherDef.initClear();
}

bool GameState::hasPendingLevelIndexChange() const
{
	return this->nextLevelIndex >= 0;
}

bool GameState::hasPendingMapDefChange() const
{
	return this->nextMapDef.isValid();
}

bool GameState::hasPendingSceneChange() const
{
	return this->hasPendingLevelIndexChange() || this->hasPendingMapDefChange();
}

void GameState::queueLevelIndexChange(int newLevelIndex, const VoxelInt2 &transitionVoxel, const VoxelInt2 &playerStartOffset)
{
	if (this->hasPendingLevelIndexChange())
	{
		DebugLogErrorFormat("Already queued level index change to level %d.", this->nextLevelIndex);
		return;
	}

	if (this->hasPendingMapDefChange())
	{
		DebugLogErrorFormat("Already changing map definition change to %d this frame.", this->nextMapDef.getMapType());
		return;
	}

	this->nextLevelIndex = newLevelIndex;
	this->nextMapLevelTransitionVoxel = transitionVoxel;
	this->nextMapPlayerStartOffset = playerStartOffset;
}

void GameState::queueMapDefChange(MapDefinition &&newMapDef, const std::optional<CoordInt2> &startCoord,
	const std::optional<CoordInt3> &returnCoord, const VoxelInt2 &playerStartOffset,
	const std::optional<WorldMapLocationIDs> &worldMapLocationIDs, bool clearPreviousMap,
	const std::optional<WeatherDefinition> &weatherDef)
{
	if (this->hasPendingMapDefChange())
	{
		DebugLogError("Already queued map definition change to " + std::to_string(static_cast<int>(this->nextMapDef.getMapType())) + ".");
		return;
	}

	if (this->hasPendingLevelIndexChange())
	{
		DebugLogError("Already changing level index to " + std::to_string(this->nextLevelIndex) + " this frame.");
		return;
	}

	this->nextMapDef = std::move(newMapDef);
	this->nextMapStartCoord = startCoord;
	this->prevMapReturnCoord = returnCoord;
	this->nextMapPlayerStartOffset = playerStartOffset;
	this->nextMapDefLocationIDs = worldMapLocationIDs;
	this->nextMapClearsPrevious = clearPreviousMap;
	this->nextMapDefWeatherDef = weatherDef;
}

void GameState::queueMapDefPop()
{
	if (this->hasPendingMapDefChange())
	{
		DebugLogError("Already queued map definition change to " + std::to_string(static_cast<int>(this->nextMapDef.getMapType())) + ".");
		return;
	}

	if (this->hasPendingLevelIndexChange())
	{
		DebugLogError("Already changing level index to " + std::to_string(this->nextLevelIndex) + " this frame.");
		return;
	}

	if (!this->isActiveMapNested())
	{
		DebugLogWarning("No exterior map to return to.");
		return;
	}

	if (!this->prevMapReturnCoord.has_value())
	{
		DebugLogWarning("Expected previous map return coord to be set.");
		return;
	}

	this->nextMapDef = std::move(this->prevMapDef);
	this->prevMapDef.clear();

	this->nextMapPlayerStartOffset = VoxelInt2::Zero;
	this->nextMapDefLocationIDs = std::nullopt;

	// Calculate weather.
	const ArenaWeatherType weatherType = this->getWeatherForLocation(this->provinceIndex, this->locationIndex);
	Random random; // @todo: get from Game
	this->nextMapDefWeatherDef = WeatherDefinition();
	this->nextMapDefWeatherDef->initFromClassic(weatherType, this->date.getDay(), random);

	this->nextMapClearsPrevious = true;
}

void GameState::queueMusicOnSceneChange(const SceneChangeMusicFunc &musicFunc, const SceneChangeMusicFunc &jingleMusicFunc)
{
	if (this->nextMusicFunc || this->nextJingleMusicFunc)
	{
		DebugLogError("Already have music queued on map change.");
		return;
	}

	this->nextMusicFunc = musicFunc;
	this->nextJingleMusicFunc = jingleMusicFunc;
}

bool GameState::hasPendingLevelTransitionCalculation() const
{
	return this->isLevelTransitionCalculationPending;
}

const CoordInt3 &GameState::getLevelTransitionCalculationPlayerCoord() const
{
	DebugAssert(this->isLevelTransitionCalculationPending);
	return this->levelTransitionCalculationPlayerCoord;
}

const CoordInt3 &GameState::getLevelTransitionCalculationTransitionCoord() const
{
	DebugAssert(this->isLevelTransitionCalculationPending);
	return this->levelTransitionCalculationTransitionCoord;
}

void GameState::queueLevelTransitionCalculation(const CoordInt3 &playerCoord, const CoordInt3 &transitionCoord)
{
	if (this->isLevelTransitionCalculationPending)
	{
		DebugLogError("Already calculating level transition.");
		return;
	}

	this->levelTransitionCalculationPlayerCoord = playerCoord;
	this->levelTransitionCalculationTransitionCoord = transitionCoord;
	this->isLevelTransitionCalculationPending = true;
}

void GameState::clearLevelTransitionCalculation()
{
	this->isLevelTransitionCalculationPending = false;
	this->levelTransitionCalculationPlayerCoord = CoordInt3();
	this->levelTransitionCalculationTransitionCoord = CoordInt3();
}

MapType GameState::getActiveMapType() const
{
	return this->getActiveMapDef().getMapType();
}

bool GameState::isActiveMapValid() const
{
	return this->activeMapDef.isValid() && (this->activeLevelIndex >= 0);
}

int GameState::getActiveLevelIndex() const
{
	return this->activeLevelIndex;
}

int GameState::getActiveSkyIndex() const
{
	if (!this->isActiveMapValid())
	{
		DebugLogError("No valid map for obtaining active sky index.");
		return -1;
	}

	return this->activeMapDef.getSkyIndexForLevel(this->activeLevelIndex);
}

const MapDefinition &GameState::getActiveMapDef() const
{
	return this->activeMapDef;
}

double GameState::getActiveCeilingScale() const
{
	if (!this->isActiveMapValid())
	{
		DebugLogError("No valid map for obtaining ceiling scale.");
		return 0.0;
	}

	const LevelInfoDefinition &levelInfoDef = this->activeMapDef.getLevelInfoForLevel(this->activeLevelIndex);
	return levelInfoDef.getCeilingScale();
}

bool GameState::isActiveMapNested() const
{
	return this->prevMapDef.isValid();
}

ArenaEnvironmentType GameState::getEnvironmentType() const
{
	ArenaEnvironmentType environmentType = ArenaEnvironmentType::City;

	const MapType mapType = this->getActiveMapType();
	if (mapType == MapType::Wilderness)
	{
		environmentType = ArenaEnvironmentType::Wilderness;
	}
	else if (mapType == MapType::Interior)
	{
		const LocationDefinition &locationDef = this->getLocationDefinition();
		const LocationDefinitionType locationType = locationDef.getType();
		if (locationType == LocationDefinitionType::MainQuestDungeon)
		{
			environmentType = ArenaEnvironmentType::MainQuestDungeon;
		}
		else if (locationType == LocationDefinitionType::Dungeon)
		{
			environmentType = ArenaEnvironmentType::WorldMapDungeon;
		}
		else if (locationType == LocationDefinitionType::City)
		{
			environmentType = ArenaEnvironmentType::BuildingInterior;
		}
	}

	return environmentType;
}

ArenaBuildingType GameState::getBuildingType() const
{
	const MapType mapType = this->getActiveMapType();
	if (mapType != MapType::Interior)
	{
		return ArenaBuildingType::None;
	}

	const MapDefinition &mapDef = this->getActiveMapDef();
	const MapSubDefinition &mapSubDef = mapDef.getSubDefinition();
	const MapDefinitionInterior &interiorDef = mapSubDef.interior;
	const ArenaInteriorType interiorType = interiorDef.interiorType;

	constexpr std::pair<ArenaInteriorType, ArenaBuildingType> interiorTypeMappings[] =
	{
		{ ArenaInteriorType::Crypt, ArenaBuildingType::Crypt },
		{ ArenaInteriorType::Dungeon, ArenaBuildingType::None },
		{ ArenaInteriorType::Equipment, ArenaBuildingType::Equipment },
		{ ArenaInteriorType::House, ArenaBuildingType::House },
		{ ArenaInteriorType::MagesGuild, ArenaBuildingType::MagesGuild },
		{ ArenaInteriorType::Noble, ArenaBuildingType::Noble },
		{ ArenaInteriorType::Palace, ArenaBuildingType::Palace },
		{ ArenaInteriorType::Tavern, ArenaBuildingType::Tavern },
		{ ArenaInteriorType::Temple, ArenaBuildingType::Temple },
		{ ArenaInteriorType::Tower, ArenaBuildingType::Tower }
	};

	const auto mappingsBegin = std::begin(interiorTypeMappings);
	const auto mappingsEnd = std::end(interiorTypeMappings);
	const auto iter = std::find_if(mappingsBegin, mappingsEnd,
		[interiorType](const std::pair<ArenaInteriorType, ArenaBuildingType> &pair)
	{
		return pair.first == interiorType;
	});

	DebugAssert(iter != mappingsEnd);
	return iter->second;
}

WorldMapInstance &GameState::getWorldMapInstance()
{
	return this->worldMapInst;
}

const ProvinceDefinition &GameState::getProvinceDefinition() const
{
	const ProvinceLibrary &provinceLibrary = ProvinceLibrary::getInstance();
	return provinceLibrary.getProvinceDef(this->provinceIndex);
}

const LocationDefinition &GameState::getLocationDefinition() const
{
	const ProvinceDefinition &provinceDef = this->getProvinceDefinition();
	return provinceDef.getLocationDef(this->locationIndex);
}

ProvinceInstance &GameState::getProvinceInstance()
{
	return this->worldMapInst.getProvinceInstance(this->provinceIndex);
}

LocationInstance &GameState::getLocationInstance()
{
	ProvinceInstance &provinceInst = this->getProvinceInstance();
	return provinceInst.getLocationInstance(this->locationIndex);
}

const ProvinceMapUiModel::TravelData *GameState::getTravelData() const
{
	return this->travelData.has_value() ? &(*this->travelData) : nullptr;
}

Span<const ArenaWeatherType> GameState::getWorldMapWeathers() const
{
	return this->worldMapWeathers;
}

ArenaWeatherType GameState::getWeatherForLocation(int provinceIndex, int locationIndex) const
{
	const ProvinceLibrary &provinceLibrary = ProvinceLibrary::getInstance();
	const ProvinceDefinition &provinceDef = provinceLibrary.getProvinceDef(provinceIndex);
	const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);
	const Int2 localPoint(locationDef.getScreenX(), locationDef.getScreenY());
	const Int2 globalPoint = ArenaLocationUtils::getGlobalPoint(localPoint, provinceDef.getGlobalRect());

	const CityDataFile &cityDataFile = BinaryAssetLibrary::getInstance().getCityDataFile();
	const int quarterIndex = ArenaLocationUtils::getGlobalQuarter(globalPoint, cityDataFile);
	DebugAssertIndex(this->worldMapWeathers, quarterIndex);
	ArenaWeatherType weatherType = this->worldMapWeathers[quarterIndex];

	if (locationDef.getType() == LocationDefinitionType::City)
	{
		// Filter the possible weathers (in case it's trying to have snow in a desert).
		const LocationCityDefinition &locationCityDef = locationDef.getCityDefinition();
		const ArenaClimateType climateType = locationCityDef.climateType;
		weatherType = ArenaWeatherUtils::getFilteredWeatherType(weatherType, climateType);
	}

	return weatherType;
}

Date &GameState::getDate()
{
	return this->date;
}

Clock &GameState::getClock()
{
	return this->clock;
}

const Clock &GameState::getClock() const
{
	return this->clock;
}

double GameState::getDayPercent() const
{
	return this->clock.getDayPercent();
}

double GameState::getChasmAnimPercent() const
{
	const double percent = this->chasmAnimSeconds / ArenaVoxelUtils::CHASM_ANIM_SECONDS;
	return std::clamp(percent, 0.0, Constants::JustBelowOne);
}

const WeatherDefinition &GameState::getWeatherDefinition() const
{
	return this->weatherDef;
}

const WeatherInstance &GameState::getWeatherInstance() const
{
	return this->weatherInst;
}

bool GameState::isFogActive() const
{
	const MapType mapType = this->getActiveMapType();
	if (mapType == MapType::Interior)
	{
		const int skyIndex = this->getActiveSkyIndex();
		const SkyInfoDefinition &skyInfoDef = this->activeMapDef.getSkyInfoForSky(skyIndex);
		return skyInfoDef.isOutdoorDungeon();
	}
	else
	{
		const bool canDaytimeFogBeActive = ArenaClockUtils::isDaytimeFogActive(this->clock);
		const WeatherType activeWeatherType = this->getWeatherDefinition().type;
		return canDaytimeFogBeActive && ((activeWeatherType == WeatherType::Overcast) || (activeWeatherType == WeatherType::Snow));
	}
}

void GameState::setIsCamping(bool isCamping)
{
	this->isCamping = isCamping;
}

void GameState::setTravelData(std::optional<ProvinceMapUiModel::TravelData> travelData)
{
	this->travelData = std::move(travelData);
}

void GameState::clearMaps()
{
	this->activeMapDef.clear();
	this->activeLevelIndex = -1;
	this->prevMapDef.clear();
	this->prevMapReturnCoord = std::nullopt;
	this->nextMapDef.clear();
	this->nextMapPlayerStartOffset = VoxelInt2::Zero;
	this->nextMapDefLocationIDs = std::nullopt;
	this->nextMapDefWeatherDef = std::nullopt;
	this->nextMapClearsPrevious = false;
	this->nextLevelIndex = -1;
	this->nextMusicFunc = SceneChangeMusicFunc();
	this->nextJingleMusicFunc = SceneChangeMusicFunc();
	this->guardSpawnState.clearQueue();
}

void GameState::updateWeatherList(ArenaRandom &random, const ExeData &exeData)
{
	const int seasonIndex = this->date.getSeason();

	const size_t weatherCount = std::size(this->worldMapWeathers);
	const auto &climates = exeData.locations.climates;
	DebugAssert(std::size(climates) == weatherCount);

	for (size_t i = 0; i < weatherCount; i++)
	{
		const int climateIndex = climates[i];
		const int variantIndex = [&random]()
		{
			// 40% for 2, 20% for 1, 20% for 3, 10% for 0, and 10% for 4.
			const int val = random.next() % 100;

			if (val >= 60)
			{
				return 2;
			}
			else if (val >= 40)
			{
				return 1;
			}
			else if (val >= 20)
			{
				return 3;
			}
			else if (val >= 10)
			{
				return 0;
			}
			else
			{
				return 4;
			}
		}();

		const int weatherTableIndex = (climateIndex * 20) + (seasonIndex * 5) + variantIndex;
		const auto &weatherTable = exeData.locations.weatherTable;
		DebugAssertIndex(weatherTable, weatherTableIndex);
		this->worldMapWeathers[i] = static_cast<ArenaWeatherType>(weatherTable[weatherTableIndex]);
	}
}

void GameState::spawnEncounterEnemies(Game &game, const EntityEncounterSpawnInfo &spawnInfo) const
{
	const Player &player = game.player;
	ArenaRandom &arenaRandom = game.arenaRandom;
	SceneManager &sceneManager = game.sceneManager;
	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	const MapType mapType = this->getActiveMapType();
	const double ceilingScale = this->getActiveCeilingScale();
	const WorldDouble3 playerFeetPosition = player.getFeetPosition();
	const WorldInt3 playerFeetVoxel = VoxelUtils::pointToVoxel(playerFeetPosition, ceilingScale);
	const OriginalInt2 playerFeetVoxelOriginalXZ = VoxelUtils::worldVoxelToOriginalVoxel(playerFeetVoxel.getXZ());
	const OriginalInt2 originalPlayerPositionArenaUnits = GameWorldUiModel::getOriginalPlayerPositionArenaUnits(playerFeetPosition, mapType);
	const int16_t originalPlayerPositionArenaUnitsX = static_cast<int16_t>(originalPlayerPositionArenaUnits.y);
	const int16_t originalPlayerPositionArenaUnitsZ = static_cast<int16_t>(originalPlayerPositionArenaUnits.x);

	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();
	const EntityDefID spawnEntityDefID = entityDefLibrary.findDefinitionIdIf(spawnInfo.entityDefPredicate);
	const EntityDefinition &spawnEntityDef = entityDefLibrary.getDefinition(spawnEntityDefID);
	const EntityAnimationDefinition &spawnAnimDef = spawnEntityDef.animDef;

	DebugAssert(spawnEntityDef.type == EntityDefinitionType::Enemy);
	const bool isCreature = spawnEntityDef.enemy.type == EnemyEntityDefinitionType::Creature;

	int actualSpawnCount = 0;
	for (int i = 0; i < spawnInfo.count; i++)
	{
		const ArenaEntitySpawnPoint spawnPositionArenaUnits = ArenaEntityUtils::findRandomSpawnLocationAroundPlayer(originalPlayerPositionArenaUnitsX, originalPlayerPositionArenaUnitsZ, arenaRandom);
		const OriginalInt2 playerToSpawnOriginalVoxel(
			(spawnPositionArenaUnits.x - originalPlayerPositionArenaUnitsX) / 128,
			(spawnPositionArenaUnits.z - originalPlayerPositionArenaUnitsZ) / 128);
		const OriginalInt2 spawnOriginalVoxel(
			playerFeetVoxelOriginalXZ.x + playerToSpawnOriginalVoxel.x,
			playerFeetVoxelOriginalXZ.y + playerToSpawnOriginalVoxel.y);
		const WorldInt2 spawnWorldVoxelXZ = VoxelUtils::originalVoxelToWorldVoxel(spawnOriginalVoxel);
		const WorldInt3 spawnWorldVoxel(spawnWorldVoxelXZ.x, 1, spawnWorldVoxelXZ.y);
		const CoordInt3 spawnVoxelCoord = VoxelUtils::worldVoxelToCoord(spawnWorldVoxel);
		const VoxelChunk *spawnVoxelChunk = voxelChunkManager.findChunkAtPosition(spawnVoxelCoord.chunk);
		if (spawnVoxelChunk == nullptr)
		{
			continue;
		}

		const VoxelInt3 spawnVoxel = spawnVoxelCoord.voxel;
		const VoxelShapeDefID spawnVoxelShapeDefID = spawnVoxelChunk->shapeDefIDs.get(spawnVoxel.x, 1, spawnVoxel.z);
		const VoxelTraitsDefID spawnFloorVoxelTraitsDefID = spawnVoxelChunk->traitsDefIDs.get(spawnVoxel.x, 0, spawnVoxel.z);
		const VoxelShapeDefinition &spawnVoxelShapeDef = spawnVoxelChunk->shapeDefs[spawnVoxelShapeDefID];
		const VoxelTraitsDefinition &spawnFloorVoxelTraitsDef = spawnVoxelChunk->traitsDefs[spawnFloorVoxelTraitsDefID];
		const bool isSpawnVoxelValid = spawnVoxelShapeDef.mesh.isEmpty() && spawnFloorVoxelTraitsDef.type == ArenaVoxelType::Floor;
		if (!isSpawnVoxelValid)
		{
			continue;
		}

		const WorldDouble2 spawnFeetPositionXZ = VoxelUtils::getVoxelCenter(spawnWorldVoxelXZ);
		const WorldDouble3 spawnFeetPosition(spawnFeetPositionXZ.x, ceilingScale, spawnFeetPositionXZ.y);

		EntityInitInfo spawnEntityInitInfo;
		spawnEntityInitInfo.defID = spawnEntityDefID;
		spawnEntityInitInfo.feetPosition = spawnFeetPosition;
		spawnEntityInitInfo.initialAnimStateIndex = *spawnAnimDef.findStateIndex(spawnAnimDef.initialStateName);
		spawnEntityInitInfo.isSensorCollider = false;
		spawnEntityInitInfo.canBeKilled = true;
		spawnEntityInitInfo.direction = CardinalDirection::North;

		if (!isCreature)
		{
			spawnEntityInitInfo.humanEnemyLevel = spawnInfo.level;
		}
		
		spawnEntityInitInfo.hasInventory = true;
		spawnEntityInitInfo.hasCreatureSound = isCreature;

		if (spawnInfo.isCityGuards())
		{
			spawnEntityInitInfo.guardType = spawnInfo.guardType;
		}

		Renderer &renderer = game.renderer;
		EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;
		entityChunkManager.createEntity(spawnEntityInitInfo, game.random, game.physicsSystem, renderer);

		RenderEntityManager &renderEntityManager = sceneManager.renderEntityManager;
		renderEntityManager.loadMaterialsForEntity(spawnEntityDefID, game.textureManager, renderer);

		if (spawnInfo.isCityGuards())
		{
			const bool isFirstSpawnedGuard = actualSpawnCount == 0;
			if (isFirstSpawnedGuard)
			{
				const WorldDouble3 spawnSoundPosition = VoxelUtils::getVoxelCenter(spawnWorldVoxel, ceilingScale);

				AudioManager &audioManager = game.audioManager;
				audioManager.playSoundOneShot(ArenaSoundName::Halt, spawnSoundPosition);
			}
		}

		actualSpawnCount++;
	}
}

void GameState::queueCityGuardEncounter(Game &game)
{
	if (this->guardSpawnState.isQueued())
	{
		return;
	}

	constexpr double originalSecondsPerFrame = 1.0 / static_cast<double>(ArenaRenderUtils::FRAMES_PER_SECOND);
	constexpr int originalGameUpdatesTillGuardSpawn = 15;
	constexpr double originalSecondsTillGuardSpawn = originalSecondsPerFrame * static_cast<double>(originalGameUpdatesTillGuardSpawn);
	this->guardSpawnState.secondsTillSpawn = originalSecondsTillGuardSpawn;

	this->guardSpawnState.spawnFunc = [this, &game]()
	{
		const Player &player = game.player;
		ArenaRandom &arenaRandom = game.arenaRandom;
		if (!ArenaEntityUtils::doGuardsAppearForViolence(player.level, arenaRandom))
		{
			DebugLog("Decided not to spawn guards.");
			return;
		}

		const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
		const LocationDefinition &locationDef = this->getLocationDefinition();
		const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
		const ArenaCityType cityType = cityDef.type;
		const int guardType = ArenaEntityUtils::getGuardType(exeData, arenaRandom);
		const int guardLevel = ArenaEntityUtils::getGuardLevel(cityType, guardType, exeData, arenaRandom);
		const int guardCount = ArenaEntityUtils::getNumberOfGuardsToSpawn(arenaRandom);

		EntityEncounterSpawnInfo encounterSpawnInfo;
		encounterSpawnInfo.initCityGuards(guardType, guardLevel, guardCount);
		this->spawnEncounterEnemies(game, encounterSpawnInfo);
	};
}

void GameState::applyPendingSceneChange(Game &game, JPH::PhysicsSystem &physicsSystem, double dt)
{
	Player &player = game.player;

	const VoxelDouble2 startOffsetReal(
		static_cast<SNDouble>(this->nextMapPlayerStartOffset.x),
		static_cast<WEDouble>(this->nextMapPlayerStartOffset.y));

	if (this->hasPendingMapDefChange())
	{
		if (!this->nextMapClearsPrevious)
		{
			this->prevMapDef = std::move(this->activeMapDef);
		}

		this->activeMapDef.clear();

		const bool shouldPopReturnCoord = this->prevMapReturnCoord.has_value() && this->nextMapClearsPrevious;
		this->nextMapClearsPrevious = false;

		if (this->nextMapDefLocationIDs.has_value())
		{
			this->provinceIndex = this->nextMapDefLocationIDs->provinceID;
			this->locationIndex = this->nextMapDefLocationIDs->locationID;
			this->nextMapDefLocationIDs = std::nullopt;
		}

		const std::optional<int> &nextMapStartLevelIndex = this->nextMapDef.getStartLevelIndex();
		this->activeLevelIndex = nextMapStartLevelIndex.value_or(0);

		this->activeMapDef = std::move(this->nextMapDef);
		this->nextMapDef.clear();

		if (this->nextMapDefWeatherDef.has_value())
		{
			this->weatherDef = std::move(*this->nextMapDefWeatherDef);
			this->nextMapDefWeatherDef = std::nullopt;
		}

		CoordDouble2 startCoord;
		if (this->nextMapStartCoord.has_value())
		{
			const VoxelInt2 startVoxelXZ = this->nextMapStartCoord->voxel;
			startCoord = CoordDouble2(this->nextMapStartCoord->chunk, VoxelUtils::getVoxelCenter(startVoxelXZ));
			this->nextMapStartCoord = std::nullopt;
		}
		else if (shouldPopReturnCoord)
		{
			const VoxelInt2 returnVoxelXZ = this->prevMapReturnCoord->voxel.getXZ();
			startCoord = CoordDouble2(this->prevMapReturnCoord->chunk, VoxelUtils::getVoxelCenter(returnVoxelXZ));
			this->prevMapReturnCoord = std::nullopt;
		}
		else if (this->activeMapDef.getStartPointCount() > 0)
		{
			const WorldDouble2 startPoint = this->activeMapDef.getStartPoint(0);
			startCoord = VoxelUtils::worldPointToCoord(startPoint);
		}
		else
		{
			DebugLogWarning("No valid start coord for map definition change.");
		}

		const double ceilingScale = this->getActiveCeilingScale();

		const CoordDouble3 newPlayerFeetCoord(
			startCoord.chunk,
			VoxelDouble3(startCoord.point.x + startOffsetReal.x, ceilingScale, startCoord.point.y + startOffsetReal.y));
		player.setPhysicsPositionRelativeToFeet(VoxelUtils::coordToWorldPoint(newPlayerFeetCoord));

		this->nextMapPlayerStartOffset = VoxelInt2::Zero;
	}
	else if (this->hasPendingLevelIndexChange())
	{
		this->activeLevelIndex = this->nextLevelIndex;
		this->nextLevelIndex = -1;

		const double ceilingScale = this->getActiveCeilingScale();

		// Can't rely on player being inside transition voxel now due to physics simulation/colliders.
		// Manually set position based on transition voxel + start offset.
		const CoordDouble3 oldPlayerEyeCoord = player.getEyeCoord();
		const ChunkInt2 oldPlayerChunk = player.getEyeCoord().chunk;
		const CoordInt2 newPlayerVoxelCoord = ChunkUtils::recalculateCoord(oldPlayerChunk, this->nextMapLevelTransitionVoxel + this->nextMapPlayerStartOffset);
		const VoxelDouble2 newPlayerPositionXZ = VoxelUtils::getVoxelCenter(newPlayerVoxelCoord.voxel);
		const CoordDouble3 newPlayerFeetCoord(
			newPlayerVoxelCoord.chunk,
			VoxelDouble3(newPlayerPositionXZ.x, ceilingScale, newPlayerPositionXZ.y));

		player.setPhysicsPositionRelativeToFeet(VoxelUtils::coordToWorldPoint(newPlayerFeetCoord));

		const WorldDouble3 newPlayerEyePosition = player.getEyePosition();
		player.lookAt(newPlayerEyePosition + Double3(startOffsetReal.x, 0.0, startOffsetReal.y));

		this->nextMapLevelTransitionVoxel = VoxelInt2::Zero;
		this->nextMapPlayerStartOffset = VoxelInt2::Zero;
	}
	else
	{
		DebugNotImplementedMsg("Unhandled scene change case.");
	}

	player.setPhysicsVelocity(Double3::Zero);

	TextureManager &textureManager = game.textureManager;
	const Window &window = game.window;
	Renderer &renderer = game.renderer;
	SceneManager &sceneManager = game.sceneManager;

	const WorldDouble3 playerPosition = player.getEyePosition();
	const ChunkInt2 playerChunk = VoxelUtils::worldPointToChunk(playerPosition);

	// Clear and re-populate scene immediately so it's ready for rendering this frame (otherwise we get a black frame).
	const Options &options = game.options;
	ChunkManager &chunkManager = sceneManager.chunkManager;
	chunkManager.clear();
	chunkManager.update(playerChunk, options.getMisc_ChunkDistance());

	sceneManager.voxelChunkManager.clear();
	sceneManager.entityChunkManager.clear(physicsSystem, renderer);
	sceneManager.voxelBoxCombineChunkManager.recycleAllChunks();
	sceneManager.voxelFaceEnableChunkManager.recycleAllChunks();
	sceneManager.voxelFaceCombineChunkManager.recycleAllChunks();
	sceneManager.collisionChunkManager.clear(physicsSystem);
	sceneManager.voxelFrustumCullingChunkManager.recycleAllChunks();
	sceneManager.entityVisChunkManager.recycleAllChunks();
	sceneManager.renderVoxelChunkManager.unloadScene(renderer);
	sceneManager.renderEntityManager.unloadScene(renderer);
	
	sceneManager.skyInstance.clear();
	sceneManager.skyVisManager.clear();
	sceneManager.renderLightManager.unloadScene(renderer);
	sceneManager.renderSkyManager.unloadScene(renderer);
	sceneManager.renderWeatherManager.unloadScene();

	const MapType activeMapType = this->getActiveMapType();
	const int activeSkyIndex = this->getActiveSkyIndex();
	const SkyDefinition &activeSkyDef = this->activeMapDef.getSky(activeSkyIndex);
	const SkyInfoDefinition &activeSkyInfoDef = this->activeMapDef.getSkyInfoForSky(activeSkyIndex);

	sceneManager.skyInstance.init(activeSkyDef, activeSkyInfoDef, this->date.getDay(), textureManager);
	sceneManager.renderEntityManager.loadScene(textureManager, renderer);
	sceneManager.renderLightManager.loadScene(renderer);
	sceneManager.renderSkyManager.loadScene(sceneManager.skyInstance, activeSkyInfoDef, textureManager, renderer);
	sceneManager.renderWeatherManager.loadScene();

	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	this->weatherInst.init(this->weatherDef, this->clock, binaryAssetLibrary.getExeData(), game.random, textureManager);

	this->guardSpawnState.clearQueue();

	const double tallPixelRatio = RendererUtils::getTallPixelRatio(options.getGraphics_TallPixelCorrection());
	RenderCamera renderCamera;
	renderCamera.init(playerPosition, player.angleX, player.angleY, options.getGraphics_VerticalFOV(), window.getSceneViewAspectRatio(), tallPixelRatio);

	constexpr bool isFloatingOriginChanged = false; // Don't need special handling when everything is already dirty.

	this->tickVoxels(0.0, game);
	this->tickEntitiesPrePhysicsStep(0.0, game);
	this->tickEntitiesPostPhysicsStep(game);
	this->tickCollision(0.0, physicsSystem, game);
	this->tickSky(0.0, game);
	this->tickVisibility(renderCamera, game);
	this->tickRendering(0.0, renderCamera, isFloatingOriginChanged, game);

	if (this->nextMusicFunc)
	{
		const MusicDefinition *musicDef = this->nextMusicFunc(game);
		const MusicDefinition *jingleMusicDef = nullptr;
		if (this->nextJingleMusicFunc)
		{
			jingleMusicDef = this->nextJingleMusicFunc(game);
		}

		AudioManager &audioManager = game.audioManager;
		audioManager.setMusic(musicDef, jingleMusicDef);

		this->nextMusicFunc = SceneChangeMusicFunc();
		this->nextJingleMusicFunc = SceneChangeMusicFunc();
	}
}

void GameState::tickGameClock(double dt, Game &game)
{
	DebugAssert(dt >= 0.0);

	const Clock prevClock = this->clock;
	const double timeScale = ArenaClockUtils::GameSecondsPerRealTimeSecond * (this->isCamping ? 250.0 : 1.0);
	this->clock.incrementTime(dt * timeScale);
	const int prevHour = prevClock.hours;
	const int newHour = this->clock.hours;

	// Check if the clock hour looped back around.
	if (newHour < prevHour)
	{
		this->date.incrementDay();
	}

	const int currentDay = this->date.getDay();
	const bool isNightForEncounters = (newHour < 6) || (newHour > 18);
	const ArenaEnvironmentType environmentType = this->getEnvironmentType();
	const ArenaBuildingType buildingType = this->getBuildingType();
	const Player &player = game.player;
	const WorldDouble3 playerPosition = player.getEyePosition();
	const MapDefinition &activeMapDef = this->getActiveMapDef();
	const MapType activeMapType = activeMapDef.getMapType();
	const EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	ArenaRandom &arenaRandom = game.arenaRandom;

	bool canAttemptEnemyEncounterThisHour = false;
	if (newHour != prevHour)
	{
		canAttemptEnemyEncounterThisHour = ArenaEntityUtils::isEnemyEncounterAllowedOnHourChanged(environmentType, this->isCamping, player.groundState.onRaisedPlatform);

		this->updateWeatherList(arenaRandom, exeData);
	}

	const int prevMinutes = prevClock.minutes;
	const int newMinutes = this->clock.minutes;
	bool canAttemptEnemyEncounterThisMinute = false;
	if (newMinutes != prevMinutes)
	{
		const bool areCitizensPresent = !isNightForEncounters || entityChunkManager.anyCitizensNearby(playerPosition);
		canAttemptEnemyEncounterThisMinute = ArenaEntityUtils::isEnemyEncounterAllowedOnMinuteChanged(environmentType, areCitizensPresent, this->isCamping, player.groundState.onRaisedPlatform);
	}

	const bool canAttemptEnemyEncounter = (canAttemptEnemyEncounterThisHour || canAttemptEnemyEncounterThisMinute) && !entityChunkManager.anyEnemiesNearby(playerPosition);
	if (canAttemptEnemyEncounter)
	{
		// Roll for random encounter.
		//@todo: The original game checks for trespassing at the moment of entering an interior, not every hour as done here.
		const bool isTrespassing = ArenaInteriorUtils::isPlayerTrespassing(buildingType, isNightForEncounters);
		const int terrainType = 0; //@todo: Pass in terrain type
		int encounterChance;
		int encounterChanceTableIndex;
		ArenaEntityUtils::getEncounterParameters(environmentType, buildingType, isTrespassing, this->isCamping, terrainType, newHour, currentDay, exeData, &encounterChance, &encounterChanceTableIndex);

		if (encounterChance > arenaRandom.next(100))
		{
			const ArenaEnemyEncounter encounter = ArenaEntityUtils::chooseEncounterEnemy(encounterChanceTableIndex, player.level, player.level, false, arenaRandom, exeData);

			EntityEncounterSpawnInfo encounterSpawnInfo;
			encounterSpawnInfo.initCreaturesOrHumans(encounter.id, encounter.level, encounter.count);
			this->spawnEncounterEnemies(game, encounterSpawnInfo);
		}
	}

	// See if the clock passed the boundary between night and day, and vice versa.
	const double oldClockTime = prevClock.getTotalSeconds();
	const double newClockTime = this->clock.getTotalSeconds();

	const ClockLibrary &clockLibrary = ClockLibrary::getInstance();
	const Clock &lamppostActivateClock = clockLibrary.getClock(ArenaClockUtils::LamppostActivate);
	const Clock &lamppostDeactivateClock = clockLibrary.getClock(ArenaClockUtils::LamppostDeactivate);
	const double lamppostActivateTime = lamppostActivateClock.getTotalSeconds();
	const double lamppostDeactivateTime = lamppostDeactivateClock.getTotalSeconds();
	const bool activateNightLights = (oldClockTime < lamppostActivateTime) && (newClockTime >= lamppostActivateTime);
	const bool deactivateNightLights = (oldClockTime < lamppostDeactivateTime) && (newClockTime >= lamppostDeactivateTime);

	if (activateNightLights)
	{
		MapLogic::handleNightLightChange(game, true);
	}
	else if (deactivateNightLights)
	{
		MapLogic::handleNightLightChange(game, false);
	}

	// Check for changes in exterior music depending on the time.
	if ((activeMapType != MapType::Interior) && !player.groundState.isSwimming)
	{
		const Clock &dayMusicStartClock = clockLibrary.getClock(ArenaClockUtils::MusicSwitchToDay);
		const Clock &nightMusicStartClock = clockLibrary.getClock(ArenaClockUtils::MusicSwitchToNight);
		const double dayMusicStartTime = dayMusicStartClock.getTotalSeconds();
		const double nightMusicStartTime = nightMusicStartClock.getTotalSeconds();
		const bool changeToDayMusic = (oldClockTime < dayMusicStartTime) && (newClockTime >= dayMusicStartTime);
		const bool changeToNightMusic = (oldClockTime < nightMusicStartTime) && (newClockTime >= nightMusicStartTime);
		
		AudioManager &audioManager = game.audioManager;
		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		const MusicDefinition *musicDef = nullptr;
		if (changeToDayMusic)
		{
			musicDef = musicLibrary.getRandomMusicDefinitionIf(MusicType::Weather, game.random,
				[this](const MusicDefinition &def)
			{
				DebugAssert(def.type == MusicType::Weather);
				const WeatherMusicDefinition &weatherMusicDef = def.weather;
				return weatherMusicDef.weatherDef == this->weatherDef;
			});

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing weather music.");
			}
		}
		else if (changeToNightMusic)
		{
			musicDef = musicLibrary.getRandomMusicDefinition(MusicType::Night, game.random);

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing night music.");
			}
		}

		if (musicDef != nullptr)
		{
			audioManager.setMusic(musicDef);
		}
	}
}

void GameState::tickChasmAnimation(double dt)
{
	this->chasmAnimSeconds += dt;
	if (this->chasmAnimSeconds >= ArenaVoxelUtils::CHASM_ANIM_SECONDS)
	{
		this->chasmAnimSeconds = std::fmod(this->chasmAnimSeconds, ArenaVoxelUtils::CHASM_ANIM_SECONDS);
	}
}

void GameState::tickSky(double dt, Game &game)
{
	SceneManager &sceneManager = game.sceneManager;
	const LocationDefinition &locationDef = this->getLocationDefinition();

	SkyInstance &skyInst = sceneManager.skyInstance;
	skyInst.update(dt, locationDef.getLatitude(), this->getDayPercent(), this->weatherInst, game.random);
}

void GameState::tickWeather(double dt, Game &game)
{
	const Window &window = game.window;
	const double windowAspectRatio = window.getAspectRatio();
	this->weatherInst.update(dt, this->clock, windowAspectRatio, game.random, game.audioManager);
}

void GameState::tickUiMessages(double dt)
{
	if (GameWorldUI::isTriggerTextVisible())
	{
		GameWorldUI::state.triggerTextRemainingSeconds  -= dt;
	}

	if (GameWorldUI::isActionTextVisible())
	{
		GameWorldUI::state.actionTextRemainingSeconds -= dt;
	}

	if (GameWorldUI::isEffectTextVisible())
	{
		GameWorldUI::state.effectTextRemainingSeconds -= dt;
	}
}

void GameState::tickPlayerHealth(double dt, Game &game)
{
	constexpr double LAVA_HEALTH_LOSS_PER_SECOND = 10.0;

	Player &player = game.player;
	double healthChange = 0.0;

	if (player.groundState.isSwimming)
	{
		const double ceilingScale = this->getActiveCeilingScale();
		const WorldDouble3 feetPosition = player.getFeetPosition();
		const CoordDouble3 feetCoord = VoxelUtils::worldPointToCoord(feetPosition);
		const CoordInt3 feetVoxelCoord = feetCoord.toVoxelScaled(ceilingScale);
		const VoxelInt3 feetVoxel = feetVoxelCoord.voxel;
		const VoxelChunkManager &voxelChunkManager = game.sceneManager.voxelChunkManager;
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(feetVoxelCoord.chunk);
		
		VoxelChasmDefID chasmDefID;
		if (voxelChunk.tryGetChasmDefID(feetVoxel.x, feetVoxel.y, feetVoxel.z, &chasmDefID))
		{
			const VoxelChasmDefinition &chasmDef = voxelChunkManager.getChasmDef(chasmDefID);
			if (chasmDef.isDamaging)
			{
				healthChange += LAVA_HEALTH_LOSS_PER_SECOND * dt;
			}
		}
	}

	player.currentHealth = std::max(player.currentHealth - healthChange, 0.0);

	if (player.currentHealth == 0.0)
	{
		GameWorldUiController::onHealthDepleted(game);
	}
}

void GameState::tickPlayerStamina(double dt, Game &game)
{
	constexpr double baseStaminaLossPerMinute = 11;
	constexpr double arenaStaminaScale = 1.0 / 64.0;
	constexpr double secondsPerMinute = 60.0;

	Player &player = game.player;

	const CharacterRaceLibrary &charRaceLibrary = CharacterRaceLibrary::getInstance();
	const CharacterRaceDefinition &charRaceDef = charRaceLibrary.getDefinition(player.raceID);

	constexpr double awakeStaminaLossPerSecond = (baseStaminaLossPerMinute * arenaStaminaScale * ArenaClockUtils::GameSecondsPerRealTimeSecond) / secondsPerMinute;
	const double swimmingStaminaLossPerSecond = awakeStaminaLossPerSecond * charRaceDef.swimmingStaminaLossMultiplier;

	double staminaChange = awakeStaminaLossPerSecond * dt;

	const bool isSwimming = player.groundState.isSwimming;
	if (isSwimming)
	{
		staminaChange += swimmingStaminaLossPerSecond * dt;
	}

	const double scaledStaminaChange = (staminaChange * 100.0) / 256.0;
	player.currentStamina = std::max(player.currentStamina - scaledStaminaChange, 0.0);

	const bool isParalyzed = player.effectsState.isParalyzed();
	if (player.currentStamina == 0.0 || (isSwimming && isParalyzed))
	{
		const bool isInterior = this->getActiveMapType() == MapType::Interior;
		const bool isNight = ArenaClockUtils::nightLightsAreActive(this->clock);
		GameWorldUiController::onStaminaExhausted(game, isSwimming, isParalyzed, isInterior, isNight);
	}
}

void GameState::tickPlayerEffects(double dt, Game &game)
{
	Player &player = game.player;
	player.effectsState.update(dt);
}

void GameState::tickPlayerEffectChanges(const PlayerEffectsState &currentEffectsState, const PlayerEffectsState &prevEffectsState)
{
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const Span<const std::string> effectNames = exeData.status.effectNames;

	std::string effectText;
	if (currentEffectsState.isDiseased() && (currentEffectsState.diseaseID != prevEffectsState.diseaseID))
	{
		effectText = GameWorldUiModel::getEffectTextBoxMessage(effectNames[0], exeData);
	}
	else if (currentEffectsState.isParalyzed() && !prevEffectsState.isParalyzed())
	{
		effectText = GameWorldUiModel::getEffectTextBoxMessage(effectNames[6], exeData);
	}

	if (!effectText.empty())
	{
		GameWorldUI::setEffectText(effectText.c_str());
	}
}

void GameState::tickPlayerAttack(double dt, Game &game)
{
	Player &player = game.player;
	player.weaponAnimInst.update(dt);

	const InputManager &inputManager = game.inputManager;

	// Use a frame-rate independent mouse delta
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 previousCombatMousePosition = inputManager.getPreviousCombatMousePosition();
	const Int2 combatMouseDelta = mousePosition - previousCombatMousePosition;

	PlayerLogic::handleAttack(game, combatMouseDelta);

	player.queuedMeleeSwingDirection = -1;
}

void GameState::tickPlayerLevel(Game &game)
{
	Player &player = game.player;
	const CharacterClassDefinition &charClassDef = CharacterClassLibrary::getInstance().getDefinition(player.charClassDefID);
	const WorldDouble3 playerPosition = player.getEyePosition();

	const EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	if (entityChunkManager.anyEnemiesNearby(playerPosition))
	{
		return;
	}

	const int prevPlayerLevel = player.level;
	while (player.experience >= charClassDef.getExperienceCap(player.level))
	{
		player.level++;
	}

	const int earnedLevelUps = player.level - prevPlayerLevel;
	const bool hasEarnedLevelUp = earnedLevelUps > 0;
	if (!hasEarnedLevelUp)
	{
		return;
	}

	Random &random = game.random;
	int bonusPoints = 0;
	for (int i = 0; i < earnedLevelUps; i++)
	{
		const int extraBonusPoints = random.next((CharacterLevelUpState::BonusPointsRandomMax - CharacterLevelUpState::BonusPointsRandomMin) + 1);
		bonusPoints += CharacterLevelUpState::BonusPointsRandomMin + extraBonusPoints;
	}
	
	AudioManager &audioManager = game.audioManager;
	audioManager.playSoundOneShot(ArenaSoundName::Fanfare1);
	DebugLogFormat("Player is now level %d with %d points to spend.", player.level, bonusPoints);

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const std::string &readyToLevelUpStr = exeData.status.readyToLevelUp;
	GameWorldUI::showTextPopUp(readyToLevelUpStr.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::MiddleCenter,
		[&game, bonusPoints]()
	{
		DebugAssert(game.charLevelUpState == nullptr);
		game.charLevelUpState = std::make_unique<CharacterLevelUpState>();
		game.charLevelUpState->init(game.player, bonusPoints);

		game.uiManager.disableTopMostContext(); // Close pop-up so the game world becomes the top-most active context for Game::handleContextChanges().
		game.setNextContext(LevelUpUI::ContextName);
	});
}

void GameState::tickVoxels(double dt, Game &game)
{
	SceneManager &sceneManager = game.sceneManager;
	const ChunkManager &chunkManager = sceneManager.chunkManager;
	const Span<const ChunkInt2> activeChunkPositions = chunkManager.getActiveChunkPositions();
	const Span<const ChunkInt2> newChunkPositions = chunkManager.getNewChunkPositions();
	const Span<const ChunkInt2> freedChunkPositions = chunkManager.getFreedChunkPositions();

	const Player &player = game.player;

	const MapDefinition &mapDef = this->getActiveMapDef();
	const int levelIndex = this->getActiveLevelIndex();
	const Span<const LevelDefinition> levelDefs = mapDef.getLevels();
	const Span<const int> levelInfoDefIndices = mapDef.getLevelInfoIndices();
	const Span<const LevelInfoDefinition> levelInfoDefs = mapDef.getLevelInfos();
	const LevelDefinition &levelDef = levelDefs[levelIndex];
	const int levelInfoIndex = levelInfoDefIndices[levelIndex];
	const LevelInfoDefinition &levelInfoDef = levelInfoDefs[levelInfoIndex];
	const MapSubDefinition &mapSubDef = mapDef.getSubDefinition();

	VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	voxelChunkManager.update(dt, newChunkPositions, freedChunkPositions, player.getEyeCoord(), &levelDef, &levelInfoDef,
		mapSubDef, levelDefs, levelInfoDefIndices, levelInfoDefs, this->getActiveCeilingScale(), game.audioManager);

	VoxelBoxCombineChunkManager &voxelBoxCombineChunkManager = sceneManager.voxelBoxCombineChunkManager;
	voxelBoxCombineChunkManager.updateActiveChunks(newChunkPositions, freedChunkPositions, voxelChunkManager);
	voxelBoxCombineChunkManager.update(activeChunkPositions, newChunkPositions, voxelChunkManager);

	VoxelFaceEnableChunkManager &voxelFaceEnableChunkManager = sceneManager.voxelFaceEnableChunkManager;
	voxelFaceEnableChunkManager.updateActiveChunks(newChunkPositions, freedChunkPositions, voxelChunkManager);
	voxelFaceEnableChunkManager.update(activeChunkPositions, newChunkPositions, voxelChunkManager);

	VoxelFaceCombineChunkManager &voxelFaceCombineChunkManager = sceneManager.voxelFaceCombineChunkManager;
	voxelFaceCombineChunkManager.updateActiveChunks(newChunkPositions, freedChunkPositions, voxelChunkManager);
	voxelFaceCombineChunkManager.update(activeChunkPositions, newChunkPositions, voxelChunkManager, voxelFaceEnableChunkManager);
}

void GameState::tickEntitiesPrePhysicsStep(double dt, Game &game)
{
	SceneManager &sceneManager = game.sceneManager;
	const ChunkManager &chunkManager = sceneManager.chunkManager;
	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;

	Player &player = game.player;

	const MapDefinition &mapDef = this->getActiveMapDef();
	const MapType mapType = mapDef.getMapType();
	const int levelIndex = this->getActiveLevelIndex();
	const Span<const LevelDefinition> levelDefs = mapDef.getLevels();
	const Span<const int> levelInfoDefIndices = mapDef.getLevelInfoIndices();
	const Span<const LevelInfoDefinition> levelInfoDefs = mapDef.getLevelInfos();
	const LevelDefinition &levelDef = levelDefs[levelIndex];
	const int levelInfoIndex = levelInfoDefIndices[levelIndex];
	const LevelInfoDefinition &levelInfoDef = levelInfoDefs[levelInfoIndex];
	const MapSubDefinition &mapSubDef = mapDef.getSubDefinition();

	const ProvinceDefinition &provinceDef = this->getProvinceDefinition();
	const LocationDefinition &locationDef = this->getLocationDefinition();
	const LocationDefinitionType locationDefType = locationDef.getType();
	const std::optional<CitizenGenInfo> citizenGenInfo = CitizenUtils::tryMakeCitizenGenInfo(mapType, provinceDef.getRaceID(), locationDef);

	// For loot generation.
	ArenaCityType lootCityType = ArenaCityType::CityState;
	ArenaInteriorType lootInteriorType = ArenaInteriorType::Crypt;

	if (locationDefType == LocationDefinitionType::City)
	{
		lootCityType = locationDef.getCityDefinition().type;
	}

	if (mapSubDef.type == MapType::Interior)
	{
		lootInteriorType = mapSubDef.interior.interiorType;
	}

	EntityGenInfo entityGenInfo;
	entityGenInfo.init(player.level, ArenaClockUtils::nightLightsAreActive(this->clock), lootCityType, lootInteriorType, levelIndex);

	const double ceilingScale = this->getActiveCeilingScale();

	EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;
	entityChunkManager.updatePrePhysicsStep(dt, chunkManager.getActiveChunkPositions(), chunkManager.getNewChunkPositions(),
		chunkManager.getFreedChunkPositions(), player, &levelDef, &levelInfoDef, mapSubDef, levelDefs, levelInfoDefIndices,
		levelInfoDefs, entityGenInfo, citizenGenInfo, ceilingScale, game.random, voxelChunkManager, game.audioManager,
		game.physicsSystem, game.textureManager, game.renderer);

	if (this->guardSpawnState.isQueued())
	{
		this->guardSpawnState.secondsTillSpawn -= dt;

		if (this->guardSpawnState.secondsTillSpawn <= 0.0)
		{
			this->guardSpawnState.spawnFunc();
			this->guardSpawnState.clearQueue();
		}
	}
}

void GameState::tickEntitiesPostPhysicsStep(Game &game)
{
	SceneManager &sceneManager = game.sceneManager;
	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;

	EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;
	entityChunkManager.updatePostPhysicsStep(voxelChunkManager, game.physicsSystem);
}

void GameState::tickCollision(double dt, JPH::PhysicsSystem &physicsSystem, Game &game)
{
	SceneManager &sceneManager = game.sceneManager;
	const ChunkManager &chunkManager = sceneManager.chunkManager;
	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	const VoxelBoxCombineChunkManager &boxCombineChunkManager = sceneManager.voxelBoxCombineChunkManager;
	const double ceilingScale = this->getActiveCeilingScale();

	CollisionChunkManager &collisionChunkManager = sceneManager.collisionChunkManager;
	collisionChunkManager.update(dt, chunkManager.getActiveChunkPositions(), chunkManager.getNewChunkPositions(),
		chunkManager.getFreedChunkPositions(), ceilingScale, voxelChunkManager, boxCombineChunkManager, physicsSystem);
}

void GameState::tickVisibility(const RenderCamera &renderCamera, Game &game)
{
	SceneManager &sceneManager = game.sceneManager;
	const ChunkManager &chunkManager = sceneManager.chunkManager;
	const Span<const ChunkInt2> activeChunkPositions = chunkManager.getActiveChunkPositions();
	const Span<const ChunkInt2> newChunkPositions = chunkManager.getNewChunkPositions();
	const Span<const ChunkInt2> freedChunkPositions = chunkManager.getFreedChunkPositions();

	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	const EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;
	const double ceilingScale = this->getActiveCeilingScale();

	VoxelFrustumCullingChunkManager &voxelFrustumCullingChunkManager = sceneManager.voxelFrustumCullingChunkManager;
	voxelFrustumCullingChunkManager.update(newChunkPositions, freedChunkPositions, renderCamera, ceilingScale, voxelChunkManager);

	EntityVisibilityChunkManager &entityVisChunkManager = sceneManager.entityVisChunkManager;
	entityVisChunkManager.update(activeChunkPositions, newChunkPositions, freedChunkPositions, renderCamera, ceilingScale,
		voxelChunkManager, entityChunkManager);

	const SkyInstance &skyInst = sceneManager.skyInstance;
	SkyVisibilityManager &skyVisManager = sceneManager.skyVisManager;
	skyVisManager.update(renderCamera, skyInst);
}

void GameState::tickRendering(double dt, const RenderCamera &renderCamera, bool isFloatingOriginChanged, Game &game)
{
	SceneManager &sceneManager = game.sceneManager;
	const ChunkManager &chunkManager = sceneManager.chunkManager;
	const Span<const ChunkInt2> activeChunkPositions = chunkManager.getActiveChunkPositions();
	const Span<const ChunkInt2> newChunkPositions = chunkManager.getNewChunkPositions();
	const Span<const ChunkInt2> freedChunkPositions = chunkManager.getFreedChunkPositions();

	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;
	const VoxelFaceCombineChunkManager &voxelFaceCombineChunkManager = sceneManager.voxelFaceCombineChunkManager;
	const SkyInstance &skyInst = sceneManager.skyInstance;

	const double ceilingScale = this->getActiveCeilingScale();
	const double chasmAnimPercent = this->getChasmAnimPercent();

	const Player &player = game.player;
	const Double2 playerDirXZ = player.getGroundDirectionXZ();

	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const bool isFoggy = this->isFogActive();
	const bool nightLightsAreActive = ArenaClockUtils::nightLightsAreActive(this->clock);	
	const Options &options = game.options;

	const VoxelFrustumCullingChunkManager &voxelFrustumCullingChunkManager = sceneManager.voxelFrustumCullingChunkManager;
	RenderVoxelChunkManager &renderVoxelChunkManager = sceneManager.renderVoxelChunkManager;
	renderVoxelChunkManager.updateActiveChunks(newChunkPositions, freedChunkPositions, voxelChunkManager, renderer);
	renderVoxelChunkManager.update(activeChunkPositions, newChunkPositions, ceilingScale, chasmAnimPercent, renderCamera.floatingOriginPoint,
		isFloatingOriginChanged, voxelChunkManager, voxelFaceCombineChunkManager, voxelFrustumCullingChunkManager, textureManager, renderer);

	const EntityVisibilityChunkManager &entityVisChunkManager = sceneManager.entityVisChunkManager;
	Span<RenderTransformHeap> entityTransformHeaps = entityChunkManager.transformHeaps;
	RenderEntityManager &renderEntityManager = sceneManager.renderEntityManager;
	renderEntityManager.update(activeChunkPositions, newChunkPositions, renderCamera, playerDirXZ, ceilingScale,
		entityChunkManager, entityVisChunkManager, entityTransformHeaps, textureManager, renderer);

	RenderLightManager &renderLightManager = sceneManager.renderLightManager;
	renderLightManager.update(renderCamera, nightLightsAreActive, isFoggy, options.getMisc_PlayerHasLight(), entityChunkManager, renderer);

	const bool isInterior = this->getActiveMapType() == MapType::Interior;
	const WeatherType weatherType = this->weatherDef.type;
	const double dayPercent = this->getDayPercent();
	sceneManager.updateGameWorldPalette(isInterior, weatherType, isFoggy, dayPercent, textureManager);

	const SkyVisibilityManager &skyVisManager = sceneManager.skyVisManager;
	const double distantAmbientPercent = ArenaRenderUtils::getDistantAmbientPercent(this->clock);
	RenderSkyManager &renderSkyManager = sceneManager.renderSkyManager;
	renderSkyManager.update(skyInst, skyVisManager, this->weatherInst, renderCamera, isInterior, dayPercent, isFoggy, distantAmbientPercent, renderer);

	const MapType mapType = this->getActiveMapType();
	const WeatherInstance &weatherInst = game.gameState.getWeatherInstance();
	RenderWeatherManager &renderWeatherManager = sceneManager.renderWeatherManager;
	renderWeatherManager.update(dt, weatherInst, renderCamera, playerDirXZ, mapType, renderer);
}
