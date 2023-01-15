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
#include "../Entities/Entity.h"
#include "../Entities/EntityManager.h"
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
#include "../WorldMap/LocationDefinition.h"
#include "../WorldMap/LocationInstance.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

GameState::WorldMapLocationIDs::WorldMapLocationIDs(int provinceID, int locationID)
{
	this->provinceID = provinceID;
	this->locationID = locationID;
}

void GameState::MapState::init(MapDefinition &&mapDefinition, MapInstance &&mapInstance,
	WeatherDefinition &&weatherDef, const std::optional<CoordInt3> &returnCoord)
{
	this->definition = std::move(mapDefinition);
	this->instance = std::move(mapInstance);
	this->weatherDef = std::move(weatherDef);
	this->returnCoord = returnCoord;
}

void GameState::MapTransitionState::init(MapState &&mapState,
	const std::optional<WorldMapLocationIDs> &worldMapLocationIDs,
	std::optional<CitizenUtils::CitizenGenInfo> &&citizenGenInfo, const CoordInt2 &startCoord,
	const std::optional<bool> &enteringInteriorFromExterior)
{
	this->mapState = std::move(mapState);
	this->worldMapLocationIDs = worldMapLocationIDs;
	this->citizenGenInfo = std::move(citizenGenInfo);
	this->startCoord = startCoord;
	this->enteringInteriorFromExterior = enteringInteriorFromExterior;
}

GameState::GameState()
{
	DebugLog("Initializing.");

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

			const bool isMainQuestDungeon = locationDef.getType() == LocationDefinition::Type::MainQuestDungeon;
			const bool isStartDungeon = isMainQuestDungeon &&
				(locationDef.getMainQuestDungeonDefinition().type == LocationDefinition::MainQuestDungeonDefinition::Type::Start);
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

	this->travelData = nullptr;
	this->nextMap = nullptr;
	this->clearMaps();
	
	this->onLevelUpVoxelEnter = std::function<void(Game&)>();

	this->weatherDef.initClear();
}

/*bool GameState::tryMakeMapFromLocation(const LocationDefinition &locationDef, int raceID, WeatherType weatherType,
	int currentDay, int starCount, bool provinceHasAnimatedLand, const CharacterClassLibrary &charClassLibrary,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	const TextAssetLibrary &textAssetLibrary, TextureManager &textureManager, MapState *outMapState)
{
	// Decide how to load and instantiate the map.
	const LocationDefinition::Type locationType = locationDef.getType();
	if (locationType == LocationDefinition::Type::City)
	{
		const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
		Buffer<uint8_t> reservedBlocks = [&cityDef]()
		{
			DebugAssert(cityDef.reservedBlocks != nullptr);
			Buffer<uint8_t> buffer(static_cast<int>(cityDef.reservedBlocks->size()));
			std::copy(cityDef.reservedBlocks->begin(), cityDef.reservedBlocks->end(), buffer.get());
			return buffer;
		}();
		
		const std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride> mainQuestTempleOverride =
			[&cityDef]() -> std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride>
		{
			if (cityDef.hasMainQuestTempleOverride)
			{
				return cityDef.mainQuestTempleOverride;
			}
			else
			{
				return std::nullopt;
			}
		}();

		MapGeneration::CityGenInfo cityGenInfo;
		cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName), cityDef.type,
			cityDef.citySeed, cityDef.rulerSeed, raceID, cityDef.premade, cityDef.coastal,
			cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks), mainQuestTempleOverride,
			cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

		SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
		skyGenInfo.init(cityDef.climateType, weatherType, currentDay, starCount, cityDef.citySeed,
			cityDef.skySeed, provinceHasAnimatedLand);

		MapDefinition mapDefinition;
		if (!mapDefinition.initCity(cityGenInfo, skyGenInfo, charClassLibrary, entityDefLibrary,
			binaryAssetLibrary, textAssetLibrary, textureManager))
		{
			DebugLogError("Couldn't init city map for location \"" + locationDef.getName() + "\".");
			return false;
		}

		MapInstance mapInstance;
		mapInstance.init(mapDefinition, textureManager);
		outMapState->init(std::move(mapDefinition), std::move(mapInstance), std::nullopt);
	}
	else if (locationType == LocationDefinition::Type::Dungeon)
	{
		const LocationDefinition::DungeonDefinition &dungeonDef = locationDef.getDungeonDefinition();

		MapGeneration::InteriorGenInfo interiorGenInfo;
		constexpr bool isArtifactDungeon = false; // @todo: not supported yet.
		interiorGenInfo.initDungeon(&dungeonDef, isArtifactDungeon);

		MapDefinition mapDefinition;
		if (!mapDefinition.initInterior(interiorGenInfo, charClassLibrary, entityDefLibrary,
			binaryAssetLibrary, textureManager))
		{
			DebugLogError("Couldn't init dungeon map for location \"" + locationDef.getName() + "\".");
			return false;
		}

		MapInstance mapInstance;
		mapInstance.init(mapDefinition, textureManager);
		outMapState->init(std::move(mapDefinition), std::move(mapInstance), std::nullopt);
	}
	else if (locationType == LocationDefinition::Type::MainQuestDungeon)
	{
		const LocationDefinition::MainQuestDungeonDefinition &mainQuestDungeonDef =
			locationDef.getMainQuestDungeonDefinition();

		MapGeneration::InteriorGenInfo interiorGenInfo;
		constexpr std::optional<bool> rulerIsMale; // Unused for main quest dungeons.
		interiorGenInfo.initPrefab(std::string(mainQuestDungeonDef.mapFilename),
			ArenaTypes::InteriorType::Dungeon, rulerIsMale);

		MapDefinition mapDefinition;
		if (!mapDefinition.initInterior(interiorGenInfo, charClassLibrary, entityDefLibrary,
			binaryAssetLibrary, textureManager))
		{
			DebugLogError("Couldn't init main quest dungeon map for location \"" + locationDef.getName() + "\".");
			return false;
		}

		MapInstance mapInstance;
		mapInstance.init(mapDefinition, textureManager);
		outMapState->init(std::move(mapDefinition), std::move(mapInstance), std::nullopt);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(locationType)));
	}

	return true;
}

bool GameState::trySetFromWorldMap(int provinceID, int locationID, const std::optional<WeatherType> &overrideWeather,
	int currentDay, int starCount, const CharacterClassLibrary &charClassLibrary,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	const TextAssetLibrary &textAssetLibrary, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssertMsg(this->nextMap == nullptr, "Already have a map to transition to.");

	// Get the province and location definitions.
	if ((provinceID < 0) || (provinceID >= this->worldMapDef.getProvinceCount()))
	{
		DebugLogError("Invalid province ID \"" + std::to_string(provinceID) + "\".");
		return false;
	}

	const ProvinceDefinition &provinceDef = this->worldMapDef.getProvinceDef(provinceID);
	if ((locationID < 0) || (locationID >= provinceDef.getLocationCount()))
	{
		DebugLogError("Invalid location ID \"" + std::to_string(locationID) + "\" for province \"" + provinceDef.getName() + "\".");
		return false;
	}

	const LocationDefinition &locationDef = provinceDef.getLocationDef(locationID);
	const int raceID = provinceDef.getRaceID();

	MapState mapState;
	if (!GameState::tryMakeMapFromLocation(locationDef, raceID, weatherType, currentDay, starCount,
		provinceDef.hasAnimatedDistantLand(), charClassLibrary, entityDefLibrary, binaryAssetLibrary,
		textAssetLibrary, textureManager, &mapState))
	{
		DebugLogError("Couldn't make map from location \"" + locationDef.getName() + "\" in province \"" + provinceDef.getName() + "\".");
		return false;
	}

	this->clearMaps();
	this->maps.emplace(std::move(mapState));

	const MapDefinition &activeMapDef = this->getActiveMapDef();
	const MapType activeMapType = activeMapDef.getMapType();
	MapInstance &activeMapInst = this->getActiveMapInst();
	const int activeLevelIndex = activeMapInst.getActiveLevelIndex();
	LevelInstance &activeLevelInst = activeMapInst.getLevel(activeLevelIndex);

	const WeatherType weatherType = [&overrideWeather, &activeMapDef]()
	{
		if (overrideWeather.has_value())
		{
			// Use this when we don't want to randomly generate the weather.
			return *overrideWeather;
		}
		else
		{
			// Determine weather from the map.
			const MapType mapType = activeMapDef.getMapType();
			if (mapType == MapType::Interior)
			{
				// Interiors are always clear.
				return WeatherType::Clear;
			}
			else if ((mapType == MapType::City) || (mapType == MapType::Wilderness))
			{
				// @todo: generate weather based on the location.
				return WeatherType::Clear;
			}
			else
			{
				DebugUnhandledReturnMsg(WeatherType, std::to_string(static_cast<int>(mapType)));
			}
		}
	}();

	DebugAssert(activeMapDef.getStartPointCount() > 0);
	const LevelDouble2 &startPoint = activeMapDef.getStartPoint(0);
	const CoordInt2 startCoord = VoxelUtils::levelVoxelToCoord(VoxelUtils::pointToVoxel(startPoint));

	const std::optional<CitizenUtils::CitizenGenInfo> citizenGenInfo = [&entityDefLibrary, &textureManager,
		&locationDef, raceID, activeMapType]() -> std::optional<CitizenUtils::CitizenGenInfo>
	{
		if ((activeMapType == MapType::City) || (activeMapType == MapType::Wilderness))
		{
			DebugAssert(locationDef.getType() == LocationDefinition::Type::City);
			const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
			const ClimateType climateType = cityDef.climateType;
			return CitizenUtils::makeCitizenGenInfo(raceID, climateType, entityDefLibrary, textureManager);
		}
		else
		{
			return std::nullopt;
		}
	}();

	// Set level active in the renderer.
	if (!this->trySetLevelActive(activeLevelInst, activeLevelIndex, weatherType, startCoord, citizenGenInfo,
		entityDefLibrary, binaryAssetLibrary, textureManager, renderer))
	{
		DebugLogError("Couldn't set level active in the renderer for location \"" + locationDef.getName() + "\".");
		return false;
	}

	// Update world map location.
	this->provinceIndex = provinceID;
	this->locationIndex = locationID;

	return true;
}*/

bool GameState::tryPushInterior(const MapGeneration::InteriorGenInfo &interiorGenInfo,
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
	const LevelDouble2 &startPoint = mapDefinition.getStartPoint(0);
	const CoordInt2 startCoord = VoxelUtils::levelVoxelToCoord(VoxelUtils::pointToVoxel(startPoint));

	// Interiors are always clear weather.
	Random weatherRandom(this->arenaRandom.getSeed()); // Cosmetic random.
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
}

bool GameState::trySetInterior(const MapGeneration::InteriorGenInfo &interiorGenInfo,
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
		const LevelDouble2 &startPoint = mapDefinition.getStartPoint(0);
		const LevelInt2 startVoxel = VoxelUtils::pointToVoxel(startPoint);
		const CoordInt2 coord = VoxelUtils::levelVoxelToCoord(startVoxel);
		const VoxelInt2 offset = playerStartOffset.has_value() ? *playerStartOffset : VoxelInt2::Zero;
		return ChunkUtils::recalculateCoord(coord.chunk, coord.voxel + offset);
	}();

	// Interiors are always clear weather.
	Random weatherRandom(this->arenaRandom.getSeed()); // Cosmetic random.
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
}

bool GameState::trySetCity(const MapGeneration::CityGenInfo &cityGenInfo,
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
	const LevelDouble2 &startPoint = mapDefinition.getStartPoint(0);
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

	const LocationDefinition::CityDefinition &cityDef = locationDefPtr->getCityDefinition();
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
}

bool GameState::trySetWilderness(const MapGeneration::WildGenInfo &wildGenInfo,
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

	const LocationDefinition::CityDefinition &cityDef = locationDefPtr->getCityDefinition();
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
}

bool GameState::tryPopMap(Player &player, const EntityDefinitionLibrary &entityDefLibrary,
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
	const MapType activeMapType = activeMapDef.getMapType();
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
			const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
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
}

const MapDefinition &GameState::getActiveMapDef() const
{
	if (this->nextMap != nullptr)
	{
		return this->nextMap->mapState.definition;
	}
	else
	{
		DebugAssert(!this->maps.empty());
		const MapState &activeMapState = this->maps.top();
		return activeMapState.definition;
	}
}

bool GameState::hasActiveMapInst() const
{
	return !this->maps.empty();
}

MapInstance &GameState::getActiveMapInst()
{
	if (this->nextMap != nullptr)
	{
		return this->nextMap->mapState.instance;
	}
	else
	{
		DebugAssert(!this->maps.empty());
		MapState &activeMapState = this->maps.top();
		return activeMapState.instance;
	}
}

const MapInstance &GameState::getActiveMapInst() const
{
	if (this->nextMap != nullptr)
	{
		return this->nextMap->mapState.instance;
	}
	else
	{
		DebugAssert(!this->maps.empty());
		const MapState &activeMapState = this->maps.top();
		return activeMapState.instance;
	}
}

bool GameState::isActiveMapNested() const
{
	return this->maps.size() >= 2;
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
	const int index = ((this->nextMap != nullptr) && this->nextMap->worldMapLocationIDs.has_value()) ?
		this->nextMap->worldMapLocationIDs->provinceID : this->provinceIndex;
	return this->worldMapDef.getProvinceDef(index);
}

const LocationDefinition &GameState::getLocationDefinition() const
{
	const ProvinceDefinition &provinceDef = this->getProvinceDefinition();
	const int index = ((this->nextMap != nullptr) && this->nextMap->worldMapLocationIDs.has_value()) ?
		this->nextMap->worldMapLocationIDs->locationID : this->locationIndex;
	return provinceDef.getLocationDef(index);
}

ProvinceInstance &GameState::getProvinceInstance()
{
	const int index = ((this->nextMap != nullptr) && this->nextMap->worldMapLocationIDs.has_value()) ?
		this->nextMap->worldMapLocationIDs->provinceID : this->provinceIndex;
	return this->worldMapInst.getProvinceInstance(index);
}

LocationInstance &GameState::getLocationInstance()
{
	ProvinceInstance &provinceInst = this->getProvinceInstance();
	const int index = ((this->nextMap != nullptr) && this->nextMap->worldMapLocationIDs.has_value()) ?
		this->nextMap->worldMapLocationIDs->locationID : this->locationIndex;
	return provinceInst.getLocationInstance(index);
}

const ProvinceMapUiModel::TravelData *GameState::getTravelData() const
{
	return this->travelData.get();
}

const GameState::WeatherList &GameState::getWeathersArray() const
{
	return this->weathers;
}

Date &GameState::getDate()
{
	return this->date;
}

Clock &GameState::getClock()
{
	return this->clock;
}

ArenaRandom &GameState::getRandom()
{
	return this->arenaRandom;
}

double GameState::getDaytimePercent() const
{
	return this->clock.getPreciseTotalSeconds() /
		static_cast<double>(Clock::SECONDS_IN_A_DAY);
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

double GameState::getAmbientPercent() const
{
	const MapDefinition *activeMapDef = nullptr;
	if (this->nextMap != nullptr)
	{
		activeMapDef = &this->nextMap->mapState.definition;
	}
	else
	{
		DebugAssert(!this->maps.empty());
		activeMapDef = &this->maps.top().definition;
	}

	DebugAssert(activeMapDef != nullptr);
	const MapType activeMapType = activeMapDef->getMapType();

	if (activeMapType == MapType::Interior)
	{
		// Completely dark indoors (some places might be an exception to this, and those
		// would be handled eventually).
		return 0.0;
	}
	else
	{
		// The ambient light outside depends on the clock time.
		const double clockPreciseSeconds = this->clock.getPreciseTotalSeconds();

		// Time ranges where the ambient light changes. The start times are inclusive,
		// and the end times are exclusive.
		const double startBrighteningTime = ArenaClockUtils::AmbientStartBrightening.getPreciseTotalSeconds();
		const double endBrighteningTime = ArenaClockUtils::AmbientEndBrightening.getPreciseTotalSeconds();
		const double startDimmingTime = ArenaClockUtils::AmbientStartDimming.getPreciseTotalSeconds();
		const double endDimmingTime = ArenaClockUtils::AmbientEndDimming.getPreciseTotalSeconds();

		// In Arena, the min ambient is 0 and the max ambient is 1, but we're using
		// some values here that make testing easier.
		constexpr double minAmbient = 0.15;
		constexpr double maxAmbient = 1.0;

		if ((clockPreciseSeconds >= endBrighteningTime) &&
			(clockPreciseSeconds < startDimmingTime))
		{
			// Daytime ambient.
			return maxAmbient;
		}
		else if ((clockPreciseSeconds >= startBrighteningTime) &&
			(clockPreciseSeconds < endBrighteningTime))
		{
			// Interpolate brightening light (in the morning).
			const double timePercent = (clockPreciseSeconds - startBrighteningTime) /
				(endBrighteningTime - startBrighteningTime);
			return minAmbient + ((maxAmbient - minAmbient) * timePercent);
		}
		else if ((clockPreciseSeconds >= startDimmingTime) &&
			(clockPreciseSeconds < endDimmingTime))
		{
			// Interpolate dimming light (in the evening).
			const double timePercent = (clockPreciseSeconds - startDimmingTime) /
				(endDimmingTime - startDimmingTime);
			return maxAmbient + ((minAmbient - maxAmbient) * timePercent);
		}
		else
		{
			// Night ambient.
			return minAmbient;
		}
	}
}

double GameState::getBetterAmbientPercent() const
{
	const double daytimePercent = this->getDaytimePercent();
	const double minAmbient = 0.20;
	const double maxAmbient = 0.90;
	const double diff = maxAmbient - minAmbient;
	const double center = minAmbient + (diff / 2.0);
	return center + ((diff / 2.0) * -std::cos(daytimePercent * (2.0 * Constants::Pi)));
}

bool GameState::nightMusicIsActive() const
{
	const double clockTime = this->clock.getPreciseTotalSeconds();
	const bool beforeDayMusicChange = clockTime < ArenaClockUtils::MusicSwitchToDay.getPreciseTotalSeconds();
	const bool afterNightMusicChange = clockTime >= ArenaClockUtils::MusicSwitchToNight.getPreciseTotalSeconds();
	return beforeDayMusicChange || afterNightMusicChange;
}

bool GameState::nightLightsAreActive() const
{
	const double clockTime = this->clock.getPreciseTotalSeconds();
	const bool beforeLamppostDeactivate = clockTime < ArenaClockUtils::LamppostDeactivate.getPreciseTotalSeconds();
	const bool afterLamppostActivate = clockTime >= ArenaClockUtils::LamppostActivate.getPreciseTotalSeconds();
	return beforeLamppostDeactivate || afterLamppostActivate;
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

void GameState::setTravelData(std::unique_ptr<ProvinceMapUiModel::TravelData> travelData)
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

bool GameState::trySetLevelActive(LevelInstance &levelInst, const std::optional<int> &activeLevelIndex,
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
}

bool GameState::trySetSkyActive(SkyInstance &skyInst, const std::optional<int> &activeLevelIndex,
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
}

bool GameState::tryApplyMapTransition(MapTransitionState &&transitionState, Player &player,
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
}

void GameState::clearMaps()
{
	while (!this->maps.empty())
	{
		this->maps.pop();
	}
}

void GameState::updateWeatherList(const ExeData &exeData)
{
	const int seasonIndex = this->date.getSeason();

	for (size_t i = 0; i < this->weathers.size(); i++)
	{
		static_assert(std::tuple_size<decltype(exeData.locations.climates)>::value ==
			std::tuple_size<decltype(this->weathers)>::value);
		
		const int climateIndex = exeData.locations.climates[i];
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
		this->weathers[i] = static_cast<ArenaTypes::WeatherType>(
			exeData.locations.weatherTable.at(weatherTableIndex));
	}
}

void GameState::tryUpdatePendingMapTransition(Game &game, double dt)
{
	Player &player = game.getPlayer();
	if (this->nextMap != nullptr)
	{
		if (!this->tryApplyMapTransition(std::move(*this->nextMap), player, game.getEntityDefinitionLibrary(),
			game.getBinaryAssetLibrary(), game.getRenderChunkManager(), game.getTextureManager(), game.getRenderer()))
		{
			DebugLogError("Couldn't apply map transition.");
		}

		this->nextMap = nullptr;

		// This mapInst.update() below is required in case we didn't do a GameWorldPanel::tick() this frame
		// (i.e. if we did a fast travel tick onAnimationFinished() kind of thing instead).
		// @todo: consider revising the Game loop more so this is handled more as a primary concern of the engine.
		const CoordDouble3 newPlayerCoord = player.getPosition();

		MapInstance &mapInst = this->getActiveMapInst();
		const double latitude = [this]()
		{
			const LocationDefinition &locationDef = this->getLocationDefinition();
			return locationDef.getLatitude();
		}();

		const EntityDefinitionLibrary &entityDefLibrary = game.getEntityDefinitionLibrary();
		TextureManager &textureManager = game.getTextureManager();

		EntityGeneration::EntityGenInfo entityGenInfo;
		entityGenInfo.init(this->nightLightsAreActive());

		// Tick active map (entities, animated distant land, etc.).
		const MapDefinition &activeMapDef = this->getActiveMapDef();
		const MapType mapType = activeMapDef.getMapType();
		const std::optional<CitizenUtils::CitizenGenInfo> citizenGenInfo = [this, &game, &entityDefLibrary,
			&textureManager, mapType]() -> std::optional<CitizenUtils::CitizenGenInfo>
		{
			if ((mapType == MapType::City) || (mapType == MapType::Wilderness))
			{
				const ProvinceDefinition &provinceDef = this->getProvinceDefinition();
				const LocationDefinition &locationDef = this->getLocationDefinition();
				const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
				return CitizenUtils::makeCitizenGenInfo(provinceDef.getRaceID(), cityDef.climateType,
					entityDefLibrary, textureManager);
			}
			else
			{
				return std::nullopt;
			}
		}();

		mapInst.update(dt, game, newPlayerCoord, activeMapDef, latitude, this->getDaytimePercent(), entityGenInfo,
			citizenGenInfo, entityDefLibrary, game.getBinaryAssetLibrary(), textureManager, game.getAudioManager());
	}
}

void GameState::tick(double dt, Game &game)
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
		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
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

	// Tick chasm animation.
	this->chasmAnimSeconds += dt;
	if (this->chasmAnimSeconds >= ArenaVoxelUtils::CHASM_ANIM_SECONDS)
	{
		this->chasmAnimSeconds = std::fmod(this->chasmAnimSeconds, ArenaVoxelUtils::CHASM_ANIM_SECONDS);
	}

	// Tick weather.
	const Renderer &renderer = game.getRenderer();
	this->weatherInst.update(dt, this->clock, renderer.getWindowAspect(), game.getRandom(), game.getAudioManager());

	// Tick on-screen text messages.
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

	// Tick the player.
	auto &player = game.getPlayer();
	const CoordDouble3 oldPlayerCoord = player.getPosition();
	player.tick(game, dt);
	const CoordDouble3 newPlayerCoord = player.getPosition();

	// Handle input for the player's attack.
	const auto &inputManager = game.getInputManager();
	const Int2 mouseDelta = inputManager.getMouseDelta();
	PlayerLogicController::handlePlayerAttack(game, mouseDelta);

	MapInstance &mapInst = this->getActiveMapInst();
	const double latitude = [this]()
	{
		const LocationDefinition &locationDef = this->getLocationDefinition();
		return locationDef.getLatitude();
	}();

	const EntityDefinitionLibrary &entityDefLibrary = game.getEntityDefinitionLibrary();
	TextureManager &textureManager = game.getTextureManager();

	EntityGeneration::EntityGenInfo entityGenInfo;
	entityGenInfo.init(this->nightLightsAreActive());

	// Tick active map (entities, animated distant land, etc.).
	const MapDefinition &mapDef = this->getActiveMapDef();
	const MapType mapType = mapDef.getMapType();
	const std::optional<CitizenUtils::CitizenGenInfo> citizenGenInfo = [this, &game, mapType,
		&entityDefLibrary, &textureManager]() -> std::optional<CitizenUtils::CitizenGenInfo>
	{
		if ((mapType == MapType::City) || (mapType == MapType::Wilderness))
		{
			const ProvinceDefinition &provinceDef = this->getProvinceDefinition();
			const LocationDefinition &locationDef = this->getLocationDefinition();
			const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
			return CitizenUtils::makeCitizenGenInfo(provinceDef.getRaceID(), cityDef.climateType,
				entityDefLibrary, textureManager);
		}
		else
		{
			return std::nullopt;
		}
	}();

	mapInst.update(dt, game, newPlayerCoord, mapDef, latitude, this->getDaytimePercent(), entityGenInfo, citizenGenInfo,
		entityDefLibrary, game.getBinaryAssetLibrary(), textureManager, game.getAudioManager());

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
	}

	// Check for changes in exterior music depending on the time.
	const MapDefinition &activeMapDef = this->getActiveMapDef();
	const MapType activeMapType = activeMapDef.getMapType();
	if ((activeMapType == MapType::City) || (activeMapType == MapType::Wilderness))
	{
		AudioManager &audioManager = game.getAudioManager();
		const MusicLibrary &musicLibrary = game.getMusicLibrary();
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
