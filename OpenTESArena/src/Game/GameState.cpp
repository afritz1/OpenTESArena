#include <algorithm>
#include <array>
#include <cmath>
#include <tuple>

#include "ArenaClockUtils.h"
#include "Game.h"
#include "GameState.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ExeData.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"
#include "../Assets/TextureManager.h"
#include "../Audio/MusicLibrary.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Entities/Player.h"
#include "../GameLogic/MapLogicController.h"
#include "../GameLogic/PlayerLogicController.h"
#include "../Interface/GameWorldUiView.h"
#include "../Math/Constants.h"
#include "../Rendering/Renderer.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../UI/TextRenderUtils.h"
#include "../Voxels/ArenaVoxelUtils.h"
#include "../Weather/ArenaWeatherUtils.h"
#include "../Weather/WeatherUtils.h"
#include "../World/MapType.h"
#include "../WorldMap/ArenaLocationUtils.h"
#include "../WorldMap/LocationDefinition.h"
#include "../WorldMap/LocationInstance.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

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

	this->triggerTextRemainingSeconds = 0.0;
	this->actionTextRemainingSeconds = 0.0;
	this->effectTextRemainingSeconds = 0.0;
	this->clearSession();
}

GameState::~GameState()
{
	DebugLog("Closing.");
}

void GameState::init(const BinaryAssetLibrary &binaryAssetLibrary)
{
	// @todo: might want a clearSession()? Seems weird.

	// Initialize world map definition and instance to default.
	this->worldMapDef.init(binaryAssetLibrary);
	this->worldMapInst.init(this->worldMapDef);

	// @temp: set main quest dungeons visible for testing.
	for (int i = 0; i < this->worldMapInst.getProvinceCount(); i++)
	{
		ProvinceInstance &provinceInst = this->worldMapInst.getProvinceInstance(i);
		const int provinceDefIndex = provinceInst.getProvinceDefIndex();
		const ProvinceDefinition &provinceDef = this->worldMapDef.getProvinceDef(provinceDefIndex);

		for (int j = 0; j < provinceInst.getLocationCount(); j++)
		{
			LocationInstance &locationInst = provinceInst.getLocationInstance(j);
			const int locationDefIndex = locationInst.getLocationDefIndex();
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
			const std::string &locationName = locationInst.getName(locationDef);

			const bool isMainQuestDungeon = locationDef.getType() == LocationDefinitionType::MainQuestDungeon;
			const bool isStartDungeon = isMainQuestDungeon &&
				(locationDef.getMainQuestDungeonDefinition().type == LocationMainQuestDungeonDefinitionType::Start);
			const bool shouldSetVisible = (locationName.size() > 0) &&
				isMainQuestDungeon && !isStartDungeon && !locationInst.isVisible();

			if (shouldSetVisible)
			{
				locationInst.toggleVisibility();
			}
		}
	}

	// Do initial weather update (to set each value to a valid state).
	this->updateWeatherList(binaryAssetLibrary.getExeData());

	this->date = Date();
	this->weatherInst = WeatherInstance();
}

void GameState::clearSession()
{
	// @todo: this function doesn't clear everything, i.e. weather state. Might want to revise later.

	// Don't have to clear on-screen text box durations.
	this->provinceIndex = -1;
	this->locationIndex = -1;

	this->isCamping = false;
	this->chasmAnimSeconds = 0.0;

	this->travelData = std::nullopt;
	this->clearMaps();
	
	this->onLevelUpVoxelEnter = std::function<void(Game&)>();

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

void GameState::queueLevelIndexChange(int newLevelIndex)
{
	if (this->hasPendingLevelIndexChange())
	{
		DebugLogError("Already queued level index change to level " + std::to_string(this->nextLevelIndex) + ".");
		return;
	}

	if (this->hasPendingMapDefChange())
	{
		DebugLogError("Already changing map definition change to " + std::to_string(static_cast<int>(this->nextMapDef.getMapType())) + " this frame.");
		return;
	}

	this->nextLevelIndex = newLevelIndex;
}

void GameState::queueMapDefChange(MapDefinition &&newMapDef, const std::optional<CoordInt3> &returnCoord,
	const VoxelInt2 &playerStartOffset, const std::optional<WorldMapLocationIDs> &worldMapLocationIDs, bool clearPreviousMap,
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
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const ProvinceDefinition &provinceDef = this->getProvinceDefinition();
	const LocationDefinition &locationDef = this->getLocationDefinition();
	const Int2 localPoint(locationDef.getScreenX(), locationDef.getScreenY());
	const Int2 globalPoint = ArenaLocationUtils::getGlobalPoint(localPoint, provinceDef.getGlobalRect());
	const int quarterIndex = ArenaLocationUtils::getGlobalQuarter(globalPoint, binaryAssetLibrary.getCityDataFile());
	DebugAssertIndex(this->worldMapWeathers, quarterIndex);
	Random random; // @todo: get from Game
	this->nextMapDefWeatherDef = WeatherDefinition();
	this->nextMapDefWeatherDef->initFromClassic(this->worldMapWeathers[quarterIndex], this->date.getDay(), random);

	this->nextMapClearsPrevious = true;

	this->nextMusicFunc = [](Game &game)
	{
		// Change to exterior music.
		GameState &gameState = game.getGameState();
		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		const Clock &clock = gameState.getClock();

		const MusicDefinition *musicDef = nullptr;
		if (!ArenaClockUtils::nightMusicIsActive(clock))
		{
			const WeatherDefinition &weatherDef = gameState.getWeatherDefinition();
			musicDef = musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
				game.getRandom(), [&weatherDef](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Weather);
				const auto &weatherMusicDef = def.getWeatherMusicDefinition();
				return weatherMusicDef.weatherDef == weatherDef;
			});
		}
		else
		{
			musicDef = musicLibrary.getRandomMusicDefinition(MusicDefinition::Type::Night, game.getRandom());
		}

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing exterior music.");
		}

		return musicDef;
	};

	this->nextJingleMusicFunc = [](Game &game)
	{
		// Only play jingle if the exterior is inside the city walls.
		GameState &gameState = game.getGameState();
		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();

		const MusicDefinition *jingleMusicDef = nullptr;
		if (gameState.getActiveMapDef().getMapType() == MapType::City)
		{
			const LocationDefinition &locationDef = gameState.getLocationDefinition();
			const LocationCityDefinition &locationCityDef = locationDef.getCityDefinition();
			jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Jingle,
				game.getRandom(), [&locationCityDef](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Jingle);
				const auto &jingleMusicDef = def.getJingleMusicDefinition();
				return (jingleMusicDef.cityType == locationCityDef.type) && (jingleMusicDef.climateType == locationCityDef.climateType);
			});

			if (jingleMusicDef == nullptr)
			{
				DebugLogWarning("Missing jingle music.");
			}
		}

		return jingleMusicDef;
	};
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

/*bool GameState::tryPushInterior(const MapGeneration::InteriorGenInfo &interiorGenInfo,
	const std::optional<CoordInt3> &returnCoord, const CharacterClassLibrary &charClassLibrary,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, Renderer &renderer)
{
	DebugAssertMsg(this->nextMap == nullptr, "Already have a map to transition to.");

	MapDefinition mapDefinition;
	if (!mapDefinition.initInterior(interiorGenInfo, charClassLibrary, entityDefLibrary,
		binaryAssetLibrary, textureManager))
	{
		DebugLogError("Couldn't init interior map from generation info.");
		return false;
	}

	constexpr int currentDay = 0; // Doesn't matter for interiors.

	MapInstance mapInstance;
	mapInstance.init(mapDefinition, currentDay, textureManager, renderer);

	// Save return voxel to the current exterior (if any).
	if (this->maps.size() > 0)
	{
		MapState &activeMapState = this->maps.top();
		activeMapState.returnCoord = returnCoord;
	}

	DebugAssert(mapDefinition.getStartPointCount() > 0);
	const WorldDouble2 &startPoint = mapDefinition.getStartPoint(0);
	const CoordInt2 startCoord = VoxelUtils::levelVoxelToCoord(VoxelUtils::pointToVoxel(startPoint));

	// Interiors are always clear weather.
	Random weatherRandom;
	WeatherDefinition weatherDef;
	weatherDef.initFromClassic(ArenaTypes::WeatherType::Clear, currentDay, weatherRandom);

	MapState mapState;
	mapState.init(std::move(mapDefinition), std::move(mapInstance), std::move(weatherDef), std::nullopt);

	const std::optional<WorldMapLocationIDs> worldMapLocationIDs; // Doesn't change when pushing an interior.
	std::optional<CitizenUtils::CitizenGenInfo> citizenGenInfo; // No citizens in interiors.
	constexpr bool enteringInteriorFromExterior = true;

	this->nextMap = std::make_unique<MapTransitionState>();
	this->nextMap->init(std::move(mapState), worldMapLocationIDs, std::move(citizenGenInfo),
		startCoord, enteringInteriorFromExterior);

	return true;
}*/

/*bool GameState::trySetInterior(const MapGeneration::InteriorGenInfo &interiorGenInfo,
	const std::optional<VoxelInt2> &playerStartOffset, const WorldMapLocationIDs &worldMapLocationIDs,
	const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssertMsg(this->nextMap == nullptr, "Already have a map to transition to.");

	MapDefinition mapDefinition;
	if (!mapDefinition.initInterior(interiorGenInfo, charClassLibrary, entityDefLibrary,
		binaryAssetLibrary, textureManager))
	{
		DebugLogError("Couldn't init interior map from generation info.");
		return false;
	}

	constexpr int currentDay = 0; // Doesn't matter for interiors.

	MapInstance mapInstance;
	mapInstance.init(mapDefinition, currentDay, textureManager, renderer);

	const CoordInt2 startCoord = [&playerStartOffset, &mapDefinition]()
	{
		DebugAssert(mapDefinition.getStartPointCount() > 0);
		const WorldDouble2 &startPoint = mapDefinition.getStartPoint(0);
		const WorldInt2 startVoxel = VoxelUtils::pointToVoxel(startPoint);
		const CoordInt2 coord = VoxelUtils::levelVoxelToCoord(startVoxel);
		const VoxelInt2 offset = playerStartOffset.has_value() ? *playerStartOffset : VoxelInt2::Zero;
		return ChunkUtils::recalculateCoord(coord.chunk, coord.voxel + offset);
	}();

	// Interiors are always clear weather.
	Random weatherRandom;
	WeatherDefinition weatherDef;
	weatherDef.initFromClassic(ArenaTypes::WeatherType::Clear, currentDay, weatherRandom);

	MapState mapState;
	mapState.init(std::move(mapDefinition), std::move(mapInstance), std::move(weatherDef), std::nullopt);

	std::optional<CitizenUtils::CitizenGenInfo> citizenGenInfo; // No citizens in interiors.
	constexpr bool enteringInteriorFromExterior = false; // This method doesn't keep an exterior alive.

	this->nextMap = std::make_unique<MapTransitionState>();
	this->nextMap->init(std::move(mapState), worldMapLocationIDs, std::move(citizenGenInfo),
		startCoord, enteringInteriorFromExterior);

	return true;
}*/

/*bool GameState::trySetCity(const MapGeneration::CityGenInfo &cityGenInfo,
	const SkyGeneration::ExteriorSkyGenInfo &skyGenInfo, const std::optional<WeatherDefinition> &overrideWeather,
	const std::optional<WorldMapLocationIDs> &newWorldMapLocationIDs,
	const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, const TextAssetLibrary &textAssetLibrary,
	TextureManager &textureManager, Renderer &renderer)
{
	DebugAssertMsg(this->nextMap == nullptr, "Already have a map to transition to.");

	MapDefinition mapDefinition;
	if (!mapDefinition.initCity(cityGenInfo, skyGenInfo, charClassLibrary, entityDefLibrary,
		binaryAssetLibrary, textAssetLibrary, textureManager))
	{
		DebugLogError("Couldn't init city map from generation info.");
		return false;
	}

	MapInstance mapInstance;
	mapInstance.init(mapDefinition, skyGenInfo.currentDay, textureManager, renderer);

	DebugAssert(mapDefinition.getStartPointCount() > 0);
	const WorldDouble2 &startPoint = mapDefinition.getStartPoint(0);
	const CoordInt2 startCoord = VoxelUtils::levelVoxelToCoord(VoxelUtils::pointToVoxel(startPoint));

	const ProvinceDefinition *provinceDefPtr = nullptr;
	const LocationDefinition *locationDefPtr = nullptr;
	if (newWorldMapLocationIDs.has_value())
	{
		provinceDefPtr = &this->worldMapDef.getProvinceDef(newWorldMapLocationIDs->provinceID);
		locationDefPtr = &provinceDefPtr->getLocationDef(newWorldMapLocationIDs->locationID);
	}
	else
	{
		// Use existing world map location (likely a wilderness->city transition).
		provinceDefPtr = &this->getProvinceDefinition();
		locationDefPtr = &this->getLocationDefinition();
	}

	const LocationCityDefinition &cityDef = locationDefPtr->getCityDefinition();
	WeatherDefinition weatherDef = [&overrideWeather, &cityDef]()
	{
		if (overrideWeather.has_value())
		{
			// Use this when we don't want to randomly generate the weather.
			return WeatherUtils::getFilteredWeather(*overrideWeather, cityDef.climateType);
		}
		else
		{
			WeatherDefinition def;
			def.initClear(); // @todo: generate the weather for this location.
			return def; 
		}
	}();

	MapState mapState;
	mapState.init(std::move(mapDefinition), std::move(mapInstance), std::move(weatherDef), std::nullopt);
	
	CitizenUtils::CitizenGenInfo citizenGenInfo = CitizenUtils::makeCitizenGenInfo(
		provinceDefPtr->getRaceID(), cityDef.climateType, entityDefLibrary, textureManager);

	constexpr std::optional<bool> enteringInteriorFromExterior; // Unused for exteriors.

	this->nextMap = std::make_unique<MapTransitionState>();
	this->nextMap->init(std::move(mapState), newWorldMapLocationIDs, std::move(citizenGenInfo),
		startCoord, enteringInteriorFromExterior);

	return true;
}*/

/*bool GameState::trySetWilderness(const MapGeneration::WildGenInfo &wildGenInfo,
	const SkyGeneration::ExteriorSkyGenInfo &skyGenInfo, const std::optional<WeatherDefinition> &overrideWeather,
	const std::optional<CoordInt3> &startCoord, const std::optional<WorldMapLocationIDs> &newWorldMapLocationIDs,
	const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssertMsg(this->nextMap == nullptr, "Already have a map to transition to.");

	// @todo: try to get gate position if current active map is for city -- need to have saved it from when the
	// gate was clicked in GameWorldPanel.
	
	MapDefinition mapDefinition;
	if (!mapDefinition.initWild(wildGenInfo, skyGenInfo, charClassLibrary, entityDefLibrary,
		binaryAssetLibrary, textureManager))
	{
		DebugLogError("Couldn't init wild map from generation info.");
		return false;
	}

	MapInstance mapInstance;
	mapInstance.init(mapDefinition, skyGenInfo.currentDay, textureManager, renderer);

	// Wilderness start point depends on city gate the player is coming out of.
	DebugAssert(mapDefinition.getStartPointCount() == 0);
	const CoordInt2 actualStartCoord = [&startCoord]()
	{
		if (startCoord.has_value())
		{
			return CoordInt2(startCoord->chunk, VoxelInt2(startCoord->voxel.x, startCoord->voxel.z));
		}
		else
		{
			// Don't have a city gate reference. Just pick somewhere in the center of the wilderness.
			return CoordInt2(
				ChunkInt2(ArenaWildUtils::WILD_WIDTH / 2, ArenaWildUtils::WILD_HEIGHT / 2),
				VoxelInt2::Zero);
		}
	}();

	const ProvinceDefinition *provinceDefPtr = nullptr;
	const LocationDefinition *locationDefPtr = nullptr;
	if (newWorldMapLocationIDs.has_value())
	{
		provinceDefPtr = &this->worldMapDef.getProvinceDef(newWorldMapLocationIDs->provinceID);
		locationDefPtr = &provinceDefPtr->getLocationDef(newWorldMapLocationIDs->locationID);
	}
	else
	{
		// Use existing world map location (likely a city->wilderness transition).
		provinceDefPtr = &this->getProvinceDefinition();
		locationDefPtr = &this->getLocationDefinition();
	}

	const LocationCityDefinition &cityDef = locationDefPtr->getCityDefinition();
	WeatherDefinition weatherDef = [&overrideWeather, &cityDef]()
	{
		if (overrideWeather.has_value())
		{
			// Use this when we don't want to randomly generate the weather.
			return WeatherUtils::getFilteredWeather(*overrideWeather, cityDef.climateType);
		}
		else
		{
			WeatherDefinition def;
			def.initClear(); // @todo: generate the weather for this location.
			return def;
		}
	}();

	MapState mapState;
	mapState.init(std::move(mapDefinition), std::move(mapInstance), std::move(weatherDef), std::nullopt);

	CitizenUtils::CitizenGenInfo citizenGenInfo = CitizenUtils::makeCitizenGenInfo(
		provinceDefPtr->getRaceID(), cityDef.climateType, entityDefLibrary, textureManager);

	constexpr std::optional<bool> enteringInteriorFromExterior; // Unused for exteriors.

	this->nextMap = std::make_unique<MapTransitionState>();
	this->nextMap->init(std::move(mapState), newWorldMapLocationIDs, std::move(citizenGenInfo),
		actualStartCoord, enteringInteriorFromExterior);

	return true;
}*/

/*bool GameState::tryPopMap(Player &player, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, RenderChunkManager &renderChunkManager, TextureManager &textureManager,
	Renderer &renderer)
{
	if (this->maps.size() == 0)
	{
		DebugLogError("No map available to pop.");
		return false;
	}

	this->maps.pop();
	if (this->maps.size() == 0)
	{
		DebugLogError("No map available to set active.");
		return false;
	}

	MapState &activeMapState = this->maps.top();
	const MapDefinition &activeMapDef = activeMapState.definition;
	const MapType activeMapType = activeMapDef.getSubDefinition().type;
	MapInstance &activeMapInst = activeMapState.instance;
	const int activeLevelIndex = activeMapInst.getActiveLevelIndex();
	LevelInstance &activeLevelInst = activeMapInst.getActiveLevel();
	SkyInstance &activeSkyInst = activeMapInst.getActiveSky();
	const std::optional<CoordInt3> &returnCoord = activeMapState.returnCoord;

	// @todo: need a condition to determine if we need to recalculate the weather (i.e., if the player slept
	// in an interior).
	WeatherDefinition activeWeatherDef = activeMapState.weatherDef;

	const CoordInt2 startCoord = [&activeMapDef, &returnCoord]()
	{
		// Use the return voxel as the start point if the now-activated map has one.
		if (returnCoord.has_value())
		{
			return CoordInt2(returnCoord->chunk, VoxelInt2(returnCoord->voxel.x, returnCoord->voxel.z));
		}
		else
		{
			// Too complex to determine (based on interior/city/wild), so just don't support for now.
			DebugUnhandledReturn(CoordInt2);
		}
	}();

	const std::optional<CitizenUtils::CitizenGenInfo> citizenGenInfo = [this, &entityDefLibrary, &textureManager,
		activeMapType]() -> std::optional<CitizenUtils::CitizenGenInfo>
	{
		if ((activeMapType == MapType::City) || (activeMapType == MapType::Wilderness))
		{
			const ProvinceDefinition &provinceDef = this->getProvinceDefinition();
			const LocationDefinition &locationDef = this->getLocationDefinition();
			const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
			return CitizenUtils::makeCitizenGenInfo(provinceDef.getRaceID(), cityDef.climateType,
				entityDefLibrary, textureManager);
		}
		else
		{
			return std::nullopt;
		}
	}();

	// Set level active in the renderer.
	if (!this->trySetLevelActive(activeLevelInst, activeLevelIndex, player, std::move(activeWeatherDef),
		startCoord, citizenGenInfo, entityDefLibrary, binaryAssetLibrary, renderChunkManager, textureManager, renderer))
	{
		DebugLogError("Couldn't set level active in the renderer for previously active level.");
		return false;
	}

	if (!this->trySetSkyActive(activeSkyInst, activeLevelIndex, textureManager, renderer))
	{
		DebugLogError("Couldn't set sky active in the renderer for previously active level.");
		return false;
	}

	return true;
}*/

MapType GameState::getActiveMapType() const
{
	return this->getActiveMapDef().getMapType();
}

bool GameState::isActiveMapValid() const
{
	return this->activeMapDef.isValid();
}

int GameState::getActiveLevelIndex() const
{
	return this->activeLevelIndex;
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

	BufferView<const LevelInfoDefinition> levelInfoDefs = this->activeMapDef.getLevelInfos();
	const LevelInfoDefinition &levelInfoDef = levelInfoDefs[this->activeLevelIndex];
	return levelInfoDef.getCeilingScale();
}

bool GameState::isActiveMapNested() const
{
	return this->prevMapDef.isValid();
}

WorldMapInstance &GameState::getWorldMapInstance()
{
	return this->worldMapInst;
}

const WorldMapDefinition &GameState::getWorldMapDefinition() const
{
	return this->worldMapDef;
}

const ProvinceDefinition &GameState::getProvinceDefinition() const
{
	int index = this->provinceIndex;
	if (this->nextMapDefLocationIDs.has_value())
	{
		index = this->nextMapDefLocationIDs->provinceID;
	}

	return this->worldMapDef.getProvinceDef(index);
}

const LocationDefinition &GameState::getLocationDefinition() const
{
	const ProvinceDefinition &provinceDef = this->getProvinceDefinition();
	int index = this->locationIndex;
	if (this->nextMapDefLocationIDs.has_value())
	{
		index = this->nextMapDefLocationIDs->locationID;
	}

	return provinceDef.getLocationDef(index);
}

ProvinceInstance &GameState::getProvinceInstance()
{
	int index = this->provinceIndex;
	if (this->nextMapDefLocationIDs.has_value())
	{
		index = this->nextMapDefLocationIDs->provinceID;
	}

	return this->worldMapInst.getProvinceInstance(index);
}

LocationInstance &GameState::getLocationInstance()
{
	ProvinceInstance &provinceInst = this->getProvinceInstance();
	int index = this->locationIndex;
	if (this->nextMapDefLocationIDs.has_value())
	{
		index = this->nextMapDefLocationIDs->locationID;
	}

	return provinceInst.getLocationInstance(index);
}

const ProvinceMapUiModel::TravelData *GameState::getTravelData() const
{
	return this->travelData.has_value() ? &(*this->travelData) : nullptr;
}

BufferView<const ArenaTypes::WeatherType> GameState::getWorldMapWeathers() const
{
	return this->worldMapWeathers;
}

Date &GameState::getDate()
{
	return this->date;
}

Clock &GameState::getClock()
{
	return this->clock;
}

double GameState::getDaytimePercent() const
{
	return this->clock.getPreciseTotalSeconds() / static_cast<double>(Clock::SECONDS_IN_A_DAY);
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

std::function<void(Game&)> &GameState::getOnLevelUpVoxelEnter()
{
	return this->onLevelUpVoxelEnter;
}

bool GameState::triggerTextIsVisible() const
{
	return this->triggerTextRemainingSeconds > 0.0;
}

bool GameState::actionTextIsVisible() const
{
	return this->actionTextRemainingSeconds > 0.0;
}

bool GameState::effectTextIsVisible() const
{
	return this->effectTextRemainingSeconds > 0.0;
}

void GameState::setIsCamping(bool isCamping)
{
	this->isCamping = isCamping;
}

void GameState::setTravelData(std::optional<ProvinceMapUiModel::TravelData> travelData)
{
	this->travelData = std::move(travelData);
}

void GameState::setTriggerTextDuration(const std::string_view &text)
{
	this->triggerTextRemainingSeconds = GameWorldUiView::getTriggerTextSeconds(text);
}

void GameState::setActionTextDuration(const std::string_view &text)
{
	this->actionTextRemainingSeconds = GameWorldUiView::getActionTextSeconds(text);
}

void GameState::setEffectTextDuration(const std::string_view &text)
{
	// @todo
	DebugNotImplemented();
}

void GameState::resetTriggerTextDuration()
{
	this->triggerTextRemainingSeconds = 0.0;
}

void GameState::resetActionTextDuration()
{
	this->actionTextRemainingSeconds = 0.0;
}

void GameState::resetEffectTextDuration()
{
	this->effectTextRemainingSeconds = 0.0;
}

/*bool GameState::trySetLevelActive(LevelInstance &levelInst, const std::optional<int> &activeLevelIndex,
	Player &player, WeatherDefinition &&weatherDef, const CoordInt2 &startCoord,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	RenderChunkManager &renderChunkManager, TextureManager &textureManager, Renderer &renderer)
{
	const VoxelDouble2 startVoxelReal = VoxelUtils::getVoxelCenter(startCoord.voxel);
	const CoordDouble3 playerPos(
		startCoord.chunk,
		VoxelDouble3(startVoxelReal.x, levelInst.getCeilingScale() + Player::HEIGHT, startVoxelReal.y));

	// Set transitioned position.
	player.teleport(playerPos);
	player.setVelocityToZero();

	this->weatherDef = std::move(weatherDef);

	Random weatherRandom; // Cosmetic random.
	this->weatherInst = WeatherInstance(); // Make sure to reset weather instance.
	this->weatherInst.init(this->weatherDef, this->clock, binaryAssetLibrary.getExeData(),
		weatherRandom, textureManager);

	DebugAssert(this->maps.size() > 0);
	const MapDefinition &mapDefinition = this->maps.top().definition;

	// @todo: need to combine setting level and sky active into a renderer.loadScene() call I think.
	if (!levelInst.trySetActive(renderChunkManager, textureManager, renderer))
	{
		DebugLogError("Couldn't set level active in the renderer.");
		return false;
	}

	return true;
}*/

/*bool GameState::trySetSkyActive(SkyInstance &skyInst, const std::optional<int> &activeLevelIndex,
	TextureManager &textureManager, Renderer &renderer)
{
	DebugAssert(this->maps.size() > 0);
	const MapDefinition &mapDefinition = this->maps.top().definition;

	if (!skyInst.trySetActive(activeLevelIndex, mapDefinition, textureManager, renderer))
	{
		DebugLogError("Couldn't set sky active in renderer.");
		return false;
	}

	return true;
}*/

/*bool GameState::tryApplyMapTransition(MapTransitionState &&transitionState, Player &player,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	RenderChunkManager &renderChunkManager, TextureManager &textureManager, Renderer &renderer)
{
	MapState &nextMapState = transitionState.mapState;
	WeatherDefinition nextWeatherDef = nextMapState.weatherDef;

	// Clear map stack if it's not entering an interior from an exterior.
	if (!transitionState.enteringInteriorFromExterior.has_value() ||
		!(*transitionState.enteringInteriorFromExterior))
	{
		this->clearMaps();
	}

	this->maps.emplace(std::move(nextMapState));

	if (transitionState.worldMapLocationIDs.has_value())
	{
		this->provinceIndex = transitionState.worldMapLocationIDs->provinceID;
		this->locationIndex = transitionState.worldMapLocationIDs->locationID;
	}

	MapInstance &newMapInst = this->maps.top().instance;
	const int newLevelInstIndex = newMapInst.getActiveLevelIndex();
	LevelInstance &newLevelInst = newMapInst.getActiveLevel();
	SkyInstance &newSkyInst = newMapInst.getActiveSky();

	if (!this->trySetLevelActive(newLevelInst, newLevelInstIndex, player, std::move(nextWeatherDef),
		transitionState.startCoord, transitionState.citizenGenInfo, entityDefLibrary, binaryAssetLibrary,
		renderChunkManager, textureManager, renderer))
	{
		DebugLogError("Couldn't set new level active.");
		return false;
	}

	if (!this->trySetSkyActive(newSkyInst, newLevelInstIndex, textureManager, renderer))
	{
		DebugLogError("Couldn't set new sky active.");
		return false;
	}

	return true;
}*/

void GameState::clearMaps()
{
	this->activeMapDef.clear();
	this->prevMapDef.clear();
	this->nextMapDef.clear();
	this->prevMapReturnCoord = std::nullopt;
	this->nextMapPlayerStartOffset = VoxelInt2::Zero;
	this->nextMapDefLocationIDs = std::nullopt;
	this->nextMapDefWeatherDef = std::nullopt;
	this->nextMapClearsPrevious = false;
	this->nextLevelIndex = -1;
	this->nextMusicFunc = SceneChangeMusicFunc();
	this->nextJingleMusicFunc = SceneChangeMusicFunc();
}

void GameState::updateWeatherList(const ExeData &exeData)
{
	const int seasonIndex = this->date.getSeason();

	const size_t weatherCount = std::size(this->worldMapWeathers);
	const auto &climates = exeData.locations.climates;
	DebugAssert(climates.size() == weatherCount);

	for (size_t i = 0; i < weatherCount; i++)
	{
		const int climateIndex = climates[i];
		const int variantIndex = [this]()
		{
			// 40% for 2, 20% for 1, 20% for 3, 10% for 0, and 10% for 4.
			const int val = this->arenaRandom.next() % 100;

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
		this->worldMapWeathers[i] = static_cast<ArenaTypes::WeatherType>(weatherTable[weatherTableIndex]);
	}
}

void GameState::tryUpdatePendingMapTransition(Game &game, double dt)
{
	Player &player = game.getPlayer();

	if (this->hasPendingMapDefChange())
	{
		DebugLog("Pending map def change.");
	}
	else if (this->hasPendingLevelIndexChange())
	{
		DebugLog("Pending level index change.");
	}

	// @todo: replace tryApplyMapTransition()
	// @todo: for level changes: if nextLevelIndex >= 0 then set activeLevelIndex to that
	// @todo: change music if nextMusicFunc is valid
	DebugLogError("tryUpdatePendingMapTransition() not implemented");

	/*if (this->nextMap != nullptr)
	{
		if (!this->tryApplyMapTransition(std::move(*this->nextMap), player, EntityDefinitionLibrary::getInstance(),
			BinaryAssetLibrary::getInstance(), game.getRenderChunkManager(), game.getTextureManager(), game.getRenderer()))
		{
			DebugLogError("Couldn't apply map transition.");
		}

		this->nextMap = nullptr;

		// This mapInst.update() below is required in case we didn't do a GameWorldPanel::tick() this frame
		// (i.e. if we did a fast travel tick onAnimationFinished() kind of thing instead).
		// @todo: consider revising the Game loop more so this is handled more as a primary concern of the engine.

		MapInstance &mapInst = this->getActiveMapInst();
		const double latitude = [this]()
		{
			const LocationDefinition &locationDef = this->getLocationDefinition();
			return locationDef.getLatitude();
		}();

		const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();
		TextureManager &textureManager = game.getTextureManager();

		EntityGeneration::EntityGenInfo entityGenInfo;
		entityGenInfo.init(ArenaClockUtils::nightLightsAreActive(this->clock));

		// Tick active map (entities, animated distant land, etc.).
		const MapDefinition &activeMapDef = this->getActiveMapDef();
		const MapType mapType = activeMapDef.getSubDefinition().type;
		const ProvinceDefinition &provinceDef = this->getProvinceDefinition();
		const LocationDefinition &locationDef = this->getLocationDefinition();
		const std::optional<CitizenUtils::CitizenGenInfo> citizenGenInfo = CitizenUtils::tryMakeCitizenGenInfo(
			mapType, provinceDef.getRaceID(), locationDef, entityDefLibrary, textureManager);

		mapInst.update(dt, game, activeMapDef, latitude, this->getDaytimePercent(), entityGenInfo, citizenGenInfo,
			entityDefLibrary, BinaryAssetLibrary::getInstance(), textureManager, game.getAudioManager());
	}*/
}

void GameState::tickGameClock(double dt, Game &game)
{
	DebugAssert(dt >= 0.0);

	// Tick the game clock.
	const Clock prevClock = this->clock;
	const double timeScale = GameState::GAME_TIME_SCALE * (this->isCamping ? 250.0 : 1.0);
	this->clock.tick(dt * timeScale);

	// Check if the hour changed.
	const int prevHour = prevClock.getHours24();
	const int newHour = this->clock.getHours24();
	if (newHour != prevHour)
	{
		// Update the weather list that's used for selecting the current one.
		const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
		this->updateWeatherList(exeData);
	}

	// Check if the clock hour looped back around.
	if (newHour < prevHour)
	{
		// Increment the day.
		this->date.incrementDay();
	}

	// See if the clock passed the boundary between night and day, and vice versa.
	const double oldClockTime = prevClock.getPreciseTotalSeconds();
	const double newClockTime = this->clock.getPreciseTotalSeconds();
	const double lamppostActivateTime = ArenaClockUtils::LamppostActivate.getPreciseTotalSeconds();
	const double lamppostDeactivateTime = ArenaClockUtils::LamppostDeactivate.getPreciseTotalSeconds();
	const bool activateNightLights = (oldClockTime < lamppostActivateTime) && (newClockTime >= lamppostActivateTime);
	const bool deactivateNightLights = (oldClockTime < lamppostDeactivateTime) && (newClockTime >= lamppostDeactivateTime);

	if (activateNightLights)
	{
		MapLogicController::handleNightLightChange(game, true);
	}
	else if (deactivateNightLights)
	{
		MapLogicController::handleNightLightChange(game, false);
	}

	// Check for changes in exterior music depending on the time.
	const MapDefinition &activeMapDef = this->getActiveMapDef();
	const MapType activeMapType = activeMapDef.getMapType();
	if ((activeMapType == MapType::City) || (activeMapType == MapType::Wilderness))
	{
		AudioManager &audioManager = game.getAudioManager();
		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		const double dayMusicStartTime = ArenaClockUtils::MusicSwitchToDay.getPreciseTotalSeconds();
		const double nightMusicStartTime = ArenaClockUtils::MusicSwitchToNight.getPreciseTotalSeconds();
		const bool changeToDayMusic = (oldClockTime < dayMusicStartTime) && (newClockTime >= dayMusicStartTime);
		const bool changeToNightMusic = (oldClockTime < nightMusicStartTime) && (newClockTime >= nightMusicStartTime);
		
		const MusicDefinition *musicDef = nullptr;
		if (changeToDayMusic)
		{
			musicDef = musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather, game.getRandom(),
				[this](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Weather);
				const auto &weatherMusicDef = def.getWeatherMusicDefinition();
				return weatherMusicDef.weatherDef == this->weatherDef;
			});

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing weather music.");
			}
		}
		else if (changeToNightMusic)
		{
			musicDef = musicLibrary.getRandomMusicDefinition(MusicDefinition::Type::Night, game.getRandom());

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

void GameState::tickWeather(double dt, Game &game)
{
	const Renderer &renderer = game.getRenderer();
	const double windowAspect = renderer.getWindowAspect();
	this->weatherInst.update(dt, this->clock, windowAspect, game.getRandom(), game.getAudioManager());
}

void GameState::tickUiMessages(double dt)
{
	if (this->triggerTextIsVisible())
	{
		this->triggerTextRemainingSeconds -= dt;
	}

	if (this->actionTextIsVisible())
	{
		this->actionTextRemainingSeconds -= dt;
	}

	if (this->effectTextIsVisible())
	{
		this->effectTextRemainingSeconds -= dt;
	}
}

void GameState::tickPlayer(double dt, Game &game)
{
	auto &player = game.getPlayer();
	const CoordDouble3 oldPlayerCoord = player.getPosition();
	player.tick(game, dt);
	const CoordDouble3 newPlayerCoord = player.getPosition();

	// Handle input for the player's attack.
	const auto &inputManager = game.getInputManager();
	const Int2 mouseDelta = inputManager.getMouseDelta();
	PlayerLogicController::handlePlayerAttack(game, mouseDelta);

	DebugLogError("Ticking mapInst in tickPlayer()");
	/*MapInstance &mapInst = this->getActiveMapInst();
	const double latitude = [this]()
	{
		const LocationDefinition &locationDef = this->getLocationDefinition();
		return locationDef.getLatitude();
	}();

	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();
	TextureManager &textureManager = game.getTextureManager();

	EntityGeneration::EntityGenInfo entityGenInfo;
	entityGenInfo.init(ArenaClockUtils::nightLightsAreActive(this->clock));

	// Tick active map (entities, animated distant land, etc.).
	const MapDefinition &mapDef = this->getActiveMapDef();
	const MapType mapType = mapDef.getSubDefinition().type;
	const ProvinceDefinition &provinceDef = this->getProvinceDefinition();
	const LocationDefinition &locationDef = this->getLocationDefinition();
	const std::optional<CitizenUtils::CitizenGenInfo> citizenGenInfo = CitizenUtils::tryMakeCitizenGenInfo(
		mapType, provinceDef.getRaceID(), locationDef, entityDefLibrary, textureManager);

	mapInst.update(dt, game, mapDef, latitude, this->getDaytimePercent(), entityGenInfo, citizenGenInfo,
		entityDefLibrary, BinaryAssetLibrary::getInstance(), textureManager, game.getAudioManager());

	// See if the player changed voxels in the XZ plane. If so, trigger text and sound events,
	// and handle any level transition.
	const LevelInstance &levelInst = mapInst.getActiveLevel();
	const double ceilingScale = levelInst.getCeilingScale();
	const CoordInt3 oldPlayerVoxelCoord(
		oldPlayerCoord.chunk, VoxelUtils::pointToVoxel(oldPlayerCoord.point, ceilingScale));
	const CoordInt3 newPlayerVoxelCoord(
		newPlayerCoord.chunk, VoxelUtils::pointToVoxel(newPlayerCoord.point, ceilingScale));
	if (newPlayerVoxelCoord != oldPlayerVoxelCoord)
	{
		TextBox *triggerTextBox = game.getTriggerTextBox();
		DebugAssert(triggerTextBox != nullptr);
		MapLogicController::handleTriggers(game, newPlayerVoxelCoord, *triggerTextBox);

		if (mapType == MapType::Interior)
		{
			MapLogicController::handleLevelTransition(game, oldPlayerVoxelCoord, newPlayerVoxelCoord);
		}
	}*/
}
