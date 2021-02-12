#include <algorithm>
#include <array>
#include <cmath>
#include <tuple>

#include "SDL.h"

#include "ArenaClockUtils.h"
#include "Game.h"
#include "GameData.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ExeData.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"
#include "../Entities/Entity.h"
#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../Interface/TextAlignment.h"
#include "../Interface/TextBox.h"
#include "../Math/Constants.h"
#include "../Media/FontName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../World/ArenaVoxelUtils.h"
#include "../World/LocationDefinition.h"
#include "../World/LocationInstance.h"
#include "../World/LocationType.h"
#include "../World/LocationUtils.h"
#include "../World/MapType.h"
#include "../World/VoxelGrid.h"
#include "../World/WeatherType.h"
#include "../World/WeatherUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	// Colors for UI text.
	const Color TriggerTextColor(215, 121, 8);
	const Color TriggerTextShadowColor(12, 12, 24);
	const Color ActionTextColor(195, 0, 0);
	const Color ActionTextShadowColor(12, 12, 24);
	const Color EffectTextColor(251, 239, 77);
	const Color EffectTextShadowColor(190, 113, 0);
}

GameData::GameData(Player &&player, const BinaryAssetLibrary &binaryAssetLibrary)
	: player(std::move(player))
{
	// Most values need to be initialized elsewhere in the program in order to determine
	// the world state, etc..
	DebugLog("Initializing.");

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
	this->updateWeather(binaryAssetLibrary.getExeData());

	this->provinceIndex = -1;
	this->locationIndex = -1;
	this->chasmAnimSeconds = 0.0;
}

GameData::~GameData()
{
	DebugLog("Closing.");
}

bool GameData::nightMusicIsActive() const
{
	const double clockTime = this->clock.getPreciseTotalSeconds();
	const bool beforeDayMusicChange = clockTime < ArenaClockUtils::MusicSwitchToDay.getPreciseTotalSeconds();
	const bool afterNightMusicChange = clockTime >= ArenaClockUtils::MusicSwitchToNight.getPreciseTotalSeconds();
	return beforeDayMusicChange || afterNightMusicChange;
}

bool GameData::nightLightsAreActive() const
{
	const double clockTime = this->clock.getPreciseTotalSeconds();
	const bool beforeLamppostDeactivate = clockTime < ArenaClockUtils::LamppostDeactivate.getPreciseTotalSeconds();
	const bool afterLamppostActivate = clockTime >= ArenaClockUtils::LamppostActivate.getPreciseTotalSeconds();
	return beforeLamppostDeactivate || afterLamppostActivate;
}

void GameData::setTransitionedPlayerPosition(const NewDouble3 &position)
{
	const CoordDouble3 coord = VoxelUtils::newPointToCoord(position);
	this->player.teleport(coord);
	this->player.setVelocityToZero();
}

void GameData::clearWorldDatas()
{
	while (!this->worldDatas.empty())
	{
		this->worldDatas.pop();
	}
}

bool GameData::loadInterior(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
	ArenaTypes::InteriorType interiorType, const MIFFile &mif, const EntityDefinitionLibrary &entityDefLibrary,
	const CharacterClassLibrary &charClassLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	Random &random, TextureManager &textureManager, Renderer &renderer)
{
	// Set location.
	if (!this->worldMapDef.tryGetProvinceIndex(provinceDef, &this->provinceIndex))
	{
		DebugLogError("Couldn't find province \"" + provinceDef.getName() + "\" in world map.");
		return false;
	}

	if (!provinceDef.tryGetLocationIndex(locationDef, &this->locationIndex))
	{
		DebugLogError("Couldn't find location \"" + locationDef.getName() + "\" in \"" + provinceDef.getName() + "\".");
		return false;
	}

	// Call interior WorldData loader.
	const auto &exeData = binaryAssetLibrary.getExeData();
	this->clearWorldDatas();
	this->worldDatas.push(std::make_unique<WorldData>(
		WorldData::loadInterior(interiorType, mif, exeData)));

	// Set initial level active in the renderer.
	WorldData &worldData = *this->worldDatas.top();
	LevelData &activeLevel = worldData.getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), worldData, this->getProvinceDefinition(),
		this->getLocationDefinition(), entityDefLibrary, charClassLibrary, binaryAssetLibrary,
		random, this->citizenManager, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = worldData.getStartPoints().front();
	this->setTransitionedPlayerPosition(NewDouble3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));

	// Arbitrary interior weather and fog.
	const double fogDistance = WeatherUtils::DEFAULT_INTERIOR_FOG_DIST;
	this->weatherType = WeatherType::Clear;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);

	return true;
}

void GameData::enterInterior(ArenaTypes::InteriorType interiorType, const MIFFile &mif, const Int2 &returnVoxel,
	const EntityDefinitionLibrary &entityDefLibrary, const CharacterClassLibrary &charClassLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, Random &random, TextureManager &textureManager,
	Renderer &renderer)
{
	DebugAssert(!this->worldDatas.empty());
	DebugAssert(this->worldDatas.top()->getMapType() != MapType::Interior);
	DebugAssert(!this->returnVoxel.has_value());

	// Give the interior world data to the active exterior.
	const auto &exeData = binaryAssetLibrary.getExeData();
	this->worldDatas.push(std::make_unique<WorldData>(
		WorldData::loadInterior(interiorType, mif, exeData)));
	this->returnVoxel = returnVoxel;

	// Set interior level active in the renderer.
	WorldData &interior = *this->worldDatas.top();
	LevelData &activeLevel = interior.getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), interior, this->getProvinceDefinition(),
		this->getLocationDefinition(), entityDefLibrary, charClassLibrary, binaryAssetLibrary,
		random, this->citizenManager, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = interior.getStartPoints().front();
	this->setTransitionedPlayerPosition(NewDouble3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));

	// Arbitrary interior fog. Do not change weather (@todo: save it maybe?).
	const double fogDistance = WeatherUtils::DEFAULT_INTERIOR_FOG_DIST;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
}

void GameData::leaveInterior(const EntityDefinitionLibrary &entityDefLibrary,
	const CharacterClassLibrary &charClassLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	Random &random, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssert(this->worldDatas.size() >= 2);
	DebugAssert(this->worldDatas.top()->getMapType() == MapType::Interior);
	DebugAssert(this->returnVoxel.has_value());

	// Remove interior world data.
	this->worldDatas.pop();

	DebugAssert(this->worldDatas.top()->getMapType() != MapType::Interior);
	WorldData &exterior = *this->worldDatas.top();

	// Leave the interior and get the voxel to return to in the exterior.
	const Int2 returnVoxel = *this->returnVoxel;
	this->returnVoxel = std::nullopt;

	// Set exterior level active in the renderer.
	LevelData &activeLevel = exterior.getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), exterior, this->getProvinceDefinition(),
		this->getLocationDefinition(), entityDefLibrary, charClassLibrary, binaryAssetLibrary,
		random, this->citizenManager, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 startPoint = VoxelUtils::getVoxelCenter(returnVoxel);
	this->setTransitionedPlayerPosition(NewDouble3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));

	// Regular sky palette based on weather.
	const Buffer<uint32_t> skyPalette =
		WeatherUtils::makeExteriorSkyPalette(this->weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.get(), skyPalette.getCount());

	// Set fog and night lights.
	const double fogDistance = WeatherUtils::getFogDistanceFromWeather(this->weatherType);
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);

	const std::string &paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette \"" + paletteName + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	renderer.setNightLightsActive(this->nightLightsAreActive(), palette);
}

bool GameData::loadNamedDungeon(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
	bool isArtifactDungeon, const EntityDefinitionLibrary &entityDefLibrary,
	const CharacterClassLibrary &charClassLibrary, const BinaryAssetLibrary &binaryAssetLibrary, Random &random,
	TextureManager &textureManager, Renderer &renderer)
{
	// Must be for a named dungeon, not main quest dungeon.
	DebugAssertMsg(locationDef.getType() == LocationDefinition::Type::Dungeon,
		"Dungeon \"" + locationDef.getName() + "\" must not be for main quest dungeon.");

	// Set location.
	if (!this->worldMapDef.tryGetProvinceIndex(provinceDef, &this->provinceIndex))
	{
		DebugLogError("Couldn't find province \"" + provinceDef.getName() + "\" in world map.");
		return false;
	}

	if (!provinceDef.tryGetLocationIndex(locationDef, &this->locationIndex))
	{
		DebugLogError("Couldn't find location \"" + locationDef.getName() + "\" in \"" + provinceDef.getName() + "\".");
		return false;
	}

	// Call dungeon WorldData loader with parameters specific to named dungeons.
	const LocationDefinition::DungeonDefinition &dungeonDef = locationDef.getDungeonDefinition();
	this->clearWorldDatas();
	this->worldDatas.push(std::make_unique<WorldData>(WorldData::loadDungeon(
		dungeonDef.dungeonSeed, dungeonDef.widthChunkCount, dungeonDef.heightChunkCount,
		isArtifactDungeon, binaryAssetLibrary.getExeData())));

	// Set initial level active in the renderer.
	WorldData &worldData = *this->worldDatas.top();
	LevelData &activeLevel = worldData.getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), worldData, this->getProvinceDefinition(),
		this->getLocationDefinition(), entityDefLibrary, charClassLibrary, binaryAssetLibrary,
		random, this->citizenManager, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = worldData.getStartPoints().front();
	this->setTransitionedPlayerPosition(NewDouble3(
		startPoint.x + 1.0, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));

	// Arbitrary interior weather and fog.
	const double fogDistance = WeatherUtils::DEFAULT_INTERIOR_FOG_DIST;
	this->weatherType = WeatherType::Clear;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);

	return true;
}

bool GameData::loadWildernessDungeon(const LocationDefinition &locationDef,
	const ProvinceDefinition &provinceDef, int wildBlockX, int wildBlockY, const CityDataFile &cityData,
	const EntityDefinitionLibrary &entityDefLibrary, const CharacterClassLibrary &charClassLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, Random &random, TextureManager &textureManager,
	Renderer &renderer)
{
	// Set location.
	if (!this->worldMapDef.tryGetProvinceIndex(provinceDef, &this->provinceIndex))
	{
		DebugLogError("Couldn't find province \"" + provinceDef.getName() + "\" in world map.");
		return false;
	}

	if (!provinceDef.tryGetLocationIndex(locationDef, &this->locationIndex))
	{
		DebugLogError("Couldn't find location \"" + locationDef.getName() + "\" in \"" + provinceDef.getName() + "\".");
		return false;
	}

	// Generate wilderness dungeon seed.
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
	const uint32_t wildDungeonSeed = cityDef.getWildDungeonSeed(wildBlockX, wildBlockY);

	// Call dungeon WorldData loader with parameters specific to wilderness dungeons.
	const auto &exeData = binaryAssetLibrary.getExeData();
	const WEInt widthChunks = LocationUtils::WILD_DUNGEON_WIDTH_CHUNK_COUNT;
	const SNInt depthChunks = LocationUtils::WILD_DUNGEON_HEIGHT_CHUNK_COUNT;
	const bool isArtifactDungeon = false;
	this->clearWorldDatas();
	this->worldDatas.push(std::make_unique<WorldData>(WorldData::loadDungeon(
		wildDungeonSeed, widthChunks, depthChunks, isArtifactDungeon, exeData)));

	// Set initial level active in the renderer.
	WorldData &worldData = *this->worldDatas.top();
	LevelData &activeLevel = worldData.getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), worldData, this->getProvinceDefinition(),
		this->getLocationDefinition(), entityDefLibrary, charClassLibrary, binaryAssetLibrary,
		random, this->citizenManager, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = worldData.getStartPoints().front();
	this->setTransitionedPlayerPosition(NewDouble3(
		startPoint.x + 1.0, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));

	// Arbitrary interior weather and fog.
	const double fogDistance = WeatherUtils::DEFAULT_INTERIOR_FOG_DIST;
	this->weatherType = WeatherType::Clear;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);

	return true;
}

bool GameData::loadCity(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
	WeatherType weatherType, int starCount, const EntityDefinitionLibrary &entityDefLibrary,
	const CharacterClassLibrary &charClassLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	const TextAssetLibrary &textAssetLibrary, Random &random, TextureManager &textureManager,
	Renderer &renderer)
{
	// Set location.
	if (!this->worldMapDef.tryGetProvinceIndex(provinceDef, &this->provinceIndex))
	{
		DebugLogError("Couldn't find province \"" + provinceDef.getName() + "\" in world map.");
		return false;
	}

	if (!provinceDef.tryGetLocationIndex(locationDef, &this->locationIndex))
	{
		DebugLogError("Couldn't find location \"" + locationDef.getName() + "\" in \"" + provinceDef.getName() + "\".");
		return false;
	}

	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
	const std::string mifName = cityDef.mapFilename;

	MIFFile mif;
	if (!mif.init(mifName.c_str()))
	{
		DebugLogError("Could not init .MIF file \"" + mifName + "\".");
		return false;
	}

	// Call city WorldData loader.
	this->clearWorldDatas();
	this->worldDatas.push(std::make_unique<WorldData>(WorldData::loadCity(
		locationDef, provinceDef, mif, weatherType, this->date.getDay(), starCount,
		binaryAssetLibrary, textAssetLibrary, textureManager)));

	// Set initial level active in the renderer.
	WorldData &worldData = *this->worldDatas.top();
	LevelData &activeLevel = worldData.getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), worldData, this->getProvinceDefinition(),
		this->getLocationDefinition(), entityDefLibrary, charClassLibrary, binaryAssetLibrary,
		random, this->citizenManager, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = worldData.getStartPoints().front();
	this->setTransitionedPlayerPosition(NewDouble3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));

	// Regular sky palette based on weather.
	const Buffer<uint32_t> skyPalette =
		WeatherUtils::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.get(), skyPalette.getCount());

	// Set weather, fog, and night lights.
	const double fogDistance = WeatherUtils::getFogDistanceFromWeather(weatherType);
	this->weatherType = weatherType;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);

	const std::string &paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette \"" + paletteName + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	renderer.setNightLightsActive(this->nightLightsAreActive(), palette);

	return true;
}

bool GameData::loadWilderness(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
	const NewInt2 &gatePos, const NewInt2 &transitionDir, bool debug_ignoreGatePos, WeatherType weatherType,
	int starCount, const EntityDefinitionLibrary &entityDefLibrary,
	const CharacterClassLibrary &charClassLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	Random &random, TextureManager &textureManager, Renderer &renderer)
{
	// Set location.
	if (!this->worldMapDef.tryGetProvinceIndex(provinceDef, &this->provinceIndex))
	{
		DebugLogError("Couldn't find province \"" + provinceDef.getName() + "\" in world map.");
		return false;
	}

	if (!provinceDef.tryGetLocationIndex(locationDef, &this->locationIndex))
	{
		DebugLogError("Couldn't find location \"" + locationDef.getName() + "\" in \"" + provinceDef.getName() + "\".");
		return false;
	}

	// Call wilderness WorldData loader.
	this->clearWorldDatas();
	this->worldDatas.push(std::make_unique<WorldData>(WorldData::loadWilderness(
		locationDef, provinceDef, weatherType, this->date.getDay(), starCount,
		binaryAssetLibrary, textureManager)));

	// Set initial level active in the renderer.
	WorldData &worldData = *this->worldDatas.top();
	LevelData &activeLevel = worldData.getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), worldData, this->getProvinceDefinition(),
		this->getLocationDefinition(), entityDefLibrary, charClassLibrary, binaryAssetLibrary,
		random, this->citizenManager, textureManager, renderer);

	// Get player starting point in the wilderness.
	const auto &voxelGrid = activeLevel.getVoxelGrid();
	const NewDouble2 startPoint = [&gatePos, &transitionDir, debug_ignoreGatePos, &voxelGrid]()
	{
		if (debug_ignoreGatePos)
		{
			// Just use center of the wilderness for testing.
			return NewDouble2(
				static_cast<SNDouble>(voxelGrid.getWidth() / 2) - 0.50,
				static_cast<WEDouble>(voxelGrid.getDepth() / 2) - 0.50);
		}
		else
		{
			// Set player starting position based on which gate they passed through. Note that the
			// original game only handles the transition one way -- going from wilderness to city
			// always uses the city's default gate instead.
			const SNInt cityStartX = RMDFile::WIDTH * 31;
			const WEInt cityStartY = RMDFile::DEPTH * 31;
			return NewDouble2(
				static_cast<SNDouble>(cityStartX + gatePos.x + transitionDir.x) + 0.50,
				static_cast<WEDouble>(cityStartY + gatePos.y + transitionDir.y) + 0.50);
		}
	}();

	this->setTransitionedPlayerPosition(NewDouble3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));

	// Regular sky palette based on weather.
	const Buffer<uint32_t> skyPalette =
		WeatherUtils::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.get(), skyPalette.getCount());

	// Set weather, fog, and night lights.
	const double fogDistance = WeatherUtils::getFogDistanceFromWeather(weatherType);
	this->weatherType = weatherType;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);

	const std::string &paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette \"" + paletteName + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	renderer.setNightLightsActive(this->nightLightsAreActive(), palette);

	return true;
}

const GameData::WeatherList &GameData::getWeathersArray() const
{
	return this->weathers;
}

Player &GameData::getPlayer()
{
	return this->player;
}

WorldData &GameData::getActiveWorld()
{
	DebugAssert(!this->worldDatas.empty());
	return *this->worldDatas.top();
}

bool GameData::isActiveWorldNested() const
{
	return this->worldDatas.size() >= 2;
}

CitizenManager &GameData::getCitizenManager()
{
	return this->citizenManager;
}

WorldMapInstance &GameData::getWorldMapInstance()
{
	return this->worldMapInst;
}

const WorldMapDefinition &GameData::getWorldMapDefinition() const
{
	return this->worldMapDef;
}

const ProvinceDefinition &GameData::getProvinceDefinition() const
{
	return this->worldMapDef.getProvinceDef(this->provinceIndex);
}

const LocationDefinition &GameData::getLocationDefinition() const
{
	const ProvinceDefinition &provinceDef = this->getProvinceDefinition();
	return provinceDef.getLocationDef(this->locationIndex);
}

ProvinceInstance &GameData::getProvinceInstance()
{
	return this->worldMapInst.getProvinceInstance(this->provinceIndex);
}

LocationInstance &GameData::getLocationInstance()
{
	ProvinceInstance &provinceInst = this->getProvinceInstance();
	return provinceInst.getLocationInstance(this->locationIndex);
}

Date &GameData::getDate()
{
	return this->date;
}

Clock &GameData::getClock()
{
	return this->clock;
}

ArenaRandom &GameData::getRandom()
{
	return this->arenaRandom;
}

double GameData::getDaytimePercent() const
{
	return this->clock.getPreciseTotalSeconds() /
		static_cast<double>(Clock::SECONDS_IN_A_DAY);
}

double GameData::getChasmAnimPercent() const
{
	const double percent = this->chasmAnimSeconds / ArenaVoxelUtils::CHASM_ANIM_SECONDS;
	return std::clamp(percent, 0.0, Constants::JustBelowOne);
}

double GameData::getFogDistance() const
{
	return this->fogDistance;
}

WeatherType GameData::getWeatherType() const
{
	return this->weatherType;
}

double GameData::getAmbientPercent() const
{
	DebugAssert(!this->worldDatas.empty());
	const WorldData &activeWorld = *this->worldDatas.top();

	if (activeWorld.getMapType() == MapType::Interior)
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

double GameData::getBetterAmbientPercent() const
{
	const double daytimePercent = this->getDaytimePercent();
	const double minAmbient = 0.20;
	const double maxAmbient = 0.90;
	const double diff = maxAmbient - minAmbient;
	const double center = minAmbient + (diff / 2.0);
	return center + ((diff / 2.0) * -std::cos(daytimePercent * (2.0 * Constants::Pi)));
}

std::function<void(Game&)> &GameData::getOnLevelUpVoxelEnter()
{
	return this->onLevelUpVoxelEnter;
}

bool GameData::triggerTextIsVisible() const
{
	return this->triggerText.hasRemainingDuration();
}

bool GameData::actionTextIsVisible() const
{
	return this->actionText.hasRemainingDuration();
}

bool GameData::effectTextIsVisible() const
{
	return this->effectText.hasRemainingDuration();
}

void GameData::getTriggerTextRenderInfo(const Texture **outTexture) const
{
	if (outTexture != nullptr)
	{
		*outTexture = &this->triggerText.textBox->getTexture();
	}
}

void GameData::getActionTextRenderInfo(const Texture **outTexture) const
{
	if (outTexture != nullptr)
	{
		*outTexture = &this->actionText.textBox->getTexture();
	}
}

void GameData::getEffectTextRenderInfo(const Texture **outTexture) const
{
	if (outTexture != nullptr)
	{
		*outTexture = &this->effectText.textBox->getTexture();
	}
}

void GameData::setTriggerText(const std::string &text, FontLibrary &fontLibrary, Renderer &renderer)
{
	const int lineSpacing = 1;
	const RichTextString richText(
		text,
		FontName::Arena,
		TriggerTextColor,
		TextAlignment::Center,
		lineSpacing,
		fontLibrary);

	const TextBox::ShadowData shadowData(TriggerTextShadowColor, Int2(-1, 0));

	// Create the text box for display (set position to zero; the renderer will
	// decide where to draw it).
	auto textBox = std::make_unique<TextBox>(
		Int2(0, 0),
		richText,
		&shadowData,
		fontLibrary,
		renderer);

	// Assign the text box and its duration to the triggered text member.
	const double duration = std::max(2.50, static_cast<double>(text.size()) * 0.050);
	this->triggerText = TimedTextBox(duration, std::move(textBox));
}

void GameData::setActionText(const std::string &text, FontLibrary &fontLibrary, Renderer &renderer)
{
	const RichTextString richText(
		text,
		FontName::Arena,
		ActionTextColor,
		TextAlignment::Center,
		fontLibrary);

	const TextBox::ShadowData shadowData(ActionTextShadowColor, Int2(-1, 0));

	// Create the text box for display (set position to zero; the renderer will decide
	// where to draw it).
	auto textBox = std::make_unique<TextBox>(
		Int2(0, 0),
		richText,
		&shadowData,
		fontLibrary,
		renderer);

	// Assign the text box and its duration to the action text.
	const double duration = std::max(2.25, static_cast<double>(text.size()) * 0.050);
	this->actionText = TimedTextBox(duration, std::move(textBox));
}

void GameData::setEffectText(const std::string &text, FontLibrary &fontLibrary, Renderer &renderer)
{
	// @todo
}

void GameData::resetTriggerText()
{
	this->triggerText.reset();
}

void GameData::resetActionText()
{
	this->actionText.reset();
}

void GameData::resetEffectText()
{
	this->effectText.reset();
}

void GameData::updateWeather(const ExeData &exeData)
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
		this->weathers[i] = static_cast<WeatherType>(
			exeData.locations.weatherTable.at(weatherTableIndex));
	}
}

void GameData::tick(double dt, Game &game)
{
	DebugAssert(dt >= 0.0);

	// Tick the game clock.
	const int oldHour = this->clock.getHours24();
	this->clock.tick(dt * GameData::TIME_SCALE);
	const int newHour = this->clock.getHours24();

	// Check if the hour changed.
	if (newHour != oldHour)
	{
		// Update the weather.
		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
		this->updateWeather(exeData);
	}

	// Check if the clock hour looped back around.
	if (newHour < oldHour)
	{
		// Increment the day.
		this->date.incrementDay();
	}

	// Tick chasm animation.
	this->chasmAnimSeconds += dt;
	if (this->chasmAnimSeconds >= ArenaVoxelUtils::CHASM_ANIM_SECONDS)
	{
		this->chasmAnimSeconds = std::fmod(this->chasmAnimSeconds, ArenaVoxelUtils::CHASM_ANIM_SECONDS);
	}

	// Tick on-screen text messages.
	auto tryTickTextBox = [dt](TimedTextBox &textBox)
	{
		if (textBox.hasRemainingDuration())
		{
			textBox.remainingDuration -= dt;
		}
	};

	tryTickTextBox(this->triggerText);
	tryTickTextBox(this->actionText);
	tryTickTextBox(this->effectText);

	// Tick citizen manager state (check if new citizens should spawn, etc.).
	this->citizenManager.tick(game);
}
