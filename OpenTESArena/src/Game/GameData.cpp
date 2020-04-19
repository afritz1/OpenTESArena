#include <algorithm>
#include <array>
#include <cmath>
#include <tuple>

#include "SDL.h"

#include "Game.h"
#include "GameData.h"
#include "../Assets/ExeData.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/Entity.h"
#include "../Entities/EntityManager.h"
#include "../Entities/GenderName.h"
#include "../Entities/Player.h"
#include "../Interface/TextAlignment.h"
#include "../Interface/TextBox.h"
#include "../Math/Constants.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../World/ExteriorWorldData.h"
#include "../World/InteriorWorldData.h"
#include "../World/LocationDefinition.h"
#include "../World/LocationInstance.h"
#include "../World/LocationType.h"
#include "../World/LocationUtils.h"
#include "../World/VoxelGrid.h"
#include "../World/WeatherType.h"
#include "../World/WeatherUtils.h"
#include "../World/WorldType.h"

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

// One real second = twenty game seconds.
const double GameData::TIME_SCALE = static_cast<double>(Clock::SECONDS_IN_A_DAY) / 4320.0;

const double GameData::CHASM_ANIM_PERIOD = 1.0 / 2.0;
const double GameData::DEFAULT_INTERIOR_FOG_DIST = 25.0;

const Clock GameData::Midnight(0, 0, 0);
const Clock GameData::Night1(0, 1, 0);
const Clock GameData::EarlyMorning(3, 0, 0);
const Clock GameData::Morning(6, 0, 0);
const Clock GameData::Noon(12, 0, 0);
const Clock GameData::Afternoon(12, 1, 0);
const Clock GameData::Evening(18, 0, 0);
const Clock GameData::Night2(21, 0, 0);

const Clock GameData::AmbientStartBrightening(6, 0, 0);
const Clock GameData::AmbientEndBrightening(6, 15, 0);
const Clock GameData::AmbientStartDimming(17, 45, 0);
const Clock GameData::AmbientEndDimming(18, 0, 0);

const Clock GameData::LamppostActivate(17, 45, 0);
const Clock GameData::LamppostDeactivate(6, 15, 0);

const Clock GameData::MusicSwitchToDay(6, 19, 0);
const Clock GameData::MusicSwitchToNight(17, 45, 0);

GameData::GameData(Player &&player, const MiscAssets &miscAssets)
	: player(std::move(player))
{
	// Most values need to be initialized elsewhere in the program in order to determine
	// the world state, etc..
	DebugLog("Initializing.");

	// Initialize world map definition and instance to default.
	this->worldMapDef.init(miscAssets);
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
	this->updateWeather(miscAssets.getExeData());

	this->provinceIndex = -1;
	this->locationIndex = -1;
	this->chasmAnimSeconds = 0.0;
}

GameData::~GameData()
{
	DebugLog("Closing.");
}

std::string GameData::getDateString(const Date &date, const ExeData &exeData)
{
	std::string text = exeData.status.date;
	
	// Replace first %s with weekday.
	const std::string &weekdayString =
		exeData.calendar.weekdayNames.at(date.getWeekday());
	size_t index = text.find("%s");
	text.replace(index, 2, weekdayString);

	// Replace %u%s with day and ordinal suffix.
	const std::string dayString = date.getOrdinalDay();
	index = text.find("%u%s");
	text.replace(index, 4, dayString);

	// Replace third %s with month.
	const std::string &monthString =
		exeData.calendar.monthNames.at(date.getMonth());
	index = text.find("%s");
	text.replace(index, 2, monthString);

	// Replace %d with year.
	index = text.find("%d");
	text.replace(index, 2, std::to_string(date.getYear()));

	return text;
}

bool GameData::nightMusicIsActive() const
{
	const double clockTime = this->clock.getPreciseTotalSeconds();
	const bool beforeDayMusicChange =
		clockTime < GameData::MusicSwitchToDay.getPreciseTotalSeconds();
	const bool afterNightMusicChange =
		clockTime >= GameData::MusicSwitchToNight.getPreciseTotalSeconds();
	return beforeDayMusicChange || afterNightMusicChange;
}

bool GameData::nightLightsAreActive() const
{
	const double clockTime = this->clock.getPreciseTotalSeconds();
	const bool beforeLamppostDeactivate =
		clockTime < GameData::LamppostDeactivate.getPreciseTotalSeconds();
	const bool afterLamppostActivate =
		clockTime >= GameData::LamppostActivate.getPreciseTotalSeconds();
	return beforeLamppostDeactivate || afterLamppostActivate;
}

bool GameData::loadInterior(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
	VoxelDefinition::WallData::MenuType interiorType, const MIFFile &mif, const MiscAssets &miscAssets,
	TextureManager &textureManager, Renderer &renderer)
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
	const auto &exeData = miscAssets.getExeData();
	this->worldData = std::make_unique<InteriorWorldData>(
		InteriorWorldData::loadInterior(interiorType, mif, exeData));

	// Set initial level active in the renderer.
	LevelData &activeLevel = this->worldData->getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), *this->worldData.get(),
		this->getLocationDefinition(), miscAssets, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = this->worldData->getStartPoints().front();
	this->player.teleport(Double3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Arbitrary interior weather and fog.
	const double fogDistance = GameData::DEFAULT_INTERIOR_FOG_DIST;
	this->weatherType = WeatherType::Clear;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);

	return true;
}

void GameData::enterInterior(VoxelDefinition::WallData::MenuType interiorType, const MIFFile &mif,
	const Int2 &returnVoxel, const MiscAssets &miscAssets, TextureManager &textureManager,
	Renderer &renderer)
{
	DebugAssert(this->worldData.get() != nullptr);
	DebugAssert(this->worldData->getActiveWorldType() != WorldType::Interior);

	ExteriorWorldData &exterior = static_cast<ExteriorWorldData&>(*this->worldData.get());
	DebugAssert(exterior.getInterior() == nullptr);

	const auto &exeData = miscAssets.getExeData();
	InteriorWorldData interior = InteriorWorldData::loadInterior(interiorType, mif, exeData);

	// Give the interior world data to the active exterior.
	exterior.enterInterior(std::move(interior), returnVoxel);

	// Set interior level active in the renderer.
	LevelData &activeLevel = exterior.getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), *exterior.getInterior(),
		this->getLocationDefinition(), miscAssets, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = exterior.getInterior()->getStartPoints().front();
	this->player.teleport(Double3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Arbitrary interior fog. Do not change weather (@todo: save it maybe?).
	const double fogDistance = GameData::DEFAULT_INTERIOR_FOG_DIST;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
}

void GameData::leaveInterior(const MiscAssets &miscAssets, TextureManager &textureManager,
	Renderer &renderer)
{
	DebugAssert(this->worldData.get() != nullptr);
	DebugAssert(this->worldData->getActiveWorldType() == WorldType::Interior);
	DebugAssert(this->worldData->getBaseWorldType() != WorldType::Interior);

	ExteriorWorldData &exterior = static_cast<ExteriorWorldData&>(*this->worldData.get());
	DebugAssert(exterior.getInterior() != nullptr);

	// Leave the interior and get the voxel to return to in the exterior.
	const Int2 returnVoxel = exterior.leaveInterior();

	// Set exterior level active in the renderer.
	LevelData &activeLevel = exterior.getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), exterior, this->getLocationDefinition(),
		miscAssets, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 startPoint(
		static_cast<double>(returnVoxel.x) + 0.50,
		static_cast<double>(returnVoxel.y) + 0.50);
	this->player.teleport(Double3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Regular sky palette based on weather.
	const Buffer<uint32_t> skyPalette =
		WeatherUtils::makeExteriorSkyPalette(this->weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.get(), skyPalette.getCount());

	// Set fog and night lights.
	const double fogDistance = WeatherUtils::getFogDistanceFromWeather(this->weatherType);
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
	renderer.setNightLightsActive(this->nightLightsAreActive());
}

bool GameData::loadNamedDungeon(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
	bool isArtifactDungeon, VoxelDefinition::WallData::MenuType interiorType,
	const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer)
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
	this->worldData = std::make_unique<InteriorWorldData>(InteriorWorldData::loadDungeon(
		dungeonDef.dungeonSeed, dungeonDef.widthChunkCount, dungeonDef.heightChunkCount,
		isArtifactDungeon, interiorType, miscAssets.getExeData()));

	// Set initial level active in the renderer.
	LevelData &activeLevel = this->worldData->getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), *this->worldData.get(),
		this->getLocationDefinition(), miscAssets, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = this->worldData->getStartPoints().front();
	this->player.teleport(Double3(
		startPoint.x - 1.0, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Arbitrary interior weather and fog.
	const double fogDistance = GameData::DEFAULT_INTERIOR_FOG_DIST;
	this->weatherType = WeatherType::Clear;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);

	return true;
}

bool GameData::loadWildernessDungeon(const LocationDefinition &locationDef,
	const ProvinceDefinition &provinceDef, int wildBlockX, int wildBlockY,
	VoxelDefinition::WallData::MenuType interiorType, const CityDataFile &cityData,
	const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer)
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
	const auto &exeData = miscAssets.getExeData();
	const WEInt widthChunks = LocationUtils::WILD_DUNGEON_WIDTH_CHUNK_COUNT;
	const SNInt depthChunks = LocationUtils::WILD_DUNGEON_HEIGHT_CHUNK_COUNT;
	const bool isArtifactDungeon = false;
	this->worldData = std::make_unique<InteriorWorldData>(InteriorWorldData::loadDungeon(
		wildDungeonSeed, widthChunks, depthChunks, isArtifactDungeon, interiorType, exeData));

	// Set initial level active in the renderer.
	LevelData &activeLevel = this->worldData->getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), *this->worldData.get(),
		this->getLocationDefinition(), miscAssets, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = this->worldData->getStartPoints().front();
	this->player.teleport(Double3(
		startPoint.x - 1.0, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Arbitrary interior weather and fog.
	const double fogDistance = GameData::DEFAULT_INTERIOR_FOG_DIST;
	this->weatherType = WeatherType::Clear;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);

	return true;
}

bool GameData::loadCity(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
	WeatherType weatherType, int starCount, const MiscAssets &miscAssets,
	TextureManager &textureManager, Renderer &renderer)
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
	const std::string mifName = cityDef.levelFilename;

	MIFFile mif;
	if (!mif.init(mifName.c_str()))
	{
		DebugLogError("Could not init .MIF file \"" + mifName + "\".");
		return false;
	}

	// Call city WorldData loader.
	this->worldData = std::make_unique<ExteriorWorldData>(ExteriorWorldData::loadCity(
		locationDef, provinceDef, mif, weatherType, this->date.getDay(), starCount,
		miscAssets, textureManager));

	// Set initial level active in the renderer.
	LevelData &activeLevel = this->worldData->getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), *this->worldData.get(),
		this->getLocationDefinition(), miscAssets, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = this->worldData->getStartPoints().front();
	this->player.teleport(Double3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Regular sky palette based on weather.
	const Buffer<uint32_t> skyPalette =
		WeatherUtils::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.get(), skyPalette.getCount());

	// Set weather, fog, and night lights.
	const double fogDistance = WeatherUtils::getFogDistanceFromWeather(weatherType);
	this->weatherType = weatherType;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
	renderer.setNightLightsActive(this->nightLightsAreActive());

	return true;
}

bool GameData::loadWilderness(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
	const Int2 &gatePos, const Int2 &transitionDir, bool debug_ignoreGatePos, WeatherType weatherType,
	int starCount, const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer)
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
	this->worldData = std::make_unique<ExteriorWorldData>(ExteriorWorldData::loadWilderness(
		locationDef, provinceDef, weatherType, this->date.getDay(), starCount,
		miscAssets, textureManager));

	// Set initial level active in the renderer.
	LevelData &activeLevel = this->worldData->getActiveLevel();
	activeLevel.setActive(this->nightLightsAreActive(), *this->worldData.get(),
		this->getLocationDefinition(), miscAssets, textureManager, renderer);

	// Get player starting point in the wilderness.
	const auto &voxelGrid = activeLevel.getVoxelGrid();
	const Double2 startPoint = [&gatePos, &transitionDir, debug_ignoreGatePos, &voxelGrid]()
	{
		if (debug_ignoreGatePos)
		{
			// Just use center of the wilderness for testing.
			return Double2(
				static_cast<double>(voxelGrid.getWidth() / 2) - 0.50,
				static_cast<double>(voxelGrid.getDepth() / 2) - 0.50);
		}
		else
		{
			// Set player starting position based on which gate they passed through. Start from
			// top-right corner of wilderness city since gatePos is assumed to be in original
			// coordinates (so size of city doesn't matter). Note that the original game only
			// handles the transition one way -- i.e., going from wilderness to city always uses
			// the city's default gate instead.
			const int cityStartX = (RMDFile::WIDTH * 33) - 1;
			const int cityStartY = (RMDFile::DEPTH * 33) - 1;
			return Double2(
				cityStartX - gatePos.y + transitionDir.x + 0.50,
				cityStartY - gatePos.x + transitionDir.y + 0.50);
		}
	}();

	this->player.teleport(Double3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Regular sky palette based on weather.
	const Buffer<uint32_t> skyPalette =
		WeatherUtils::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.get(), skyPalette.getCount());

	// Set weather, fog, and night lights.
	const double fogDistance = WeatherUtils::getFogDistanceFromWeather(weatherType);
	this->weatherType = weatherType;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
	renderer.setNightLightsActive(this->nightLightsAreActive());

	return true;
}

const std::array<WeatherType, 36> &GameData::getWeathersArray() const
{
	return this->weathers;
}

Player &GameData::getPlayer()
{
	return this->player;
}

WorldData &GameData::getWorldData()
{
	DebugAssert(this->worldData.get() != nullptr);
	return *this->worldData.get();
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
	const double percent = this->chasmAnimSeconds / GameData::CHASM_ANIM_PERIOD;
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
	if (this->worldData->getActiveWorldType() == WorldType::Interior)
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
		const double startBrighteningTime = GameData::AmbientStartBrightening.getPreciseTotalSeconds();
		const double endBrighteningTime = GameData::AmbientEndBrightening.getPreciseTotalSeconds();
		const double startDimmingTime = GameData::AmbientStartDimming.getPreciseTotalSeconds();
		const double endDimmingTime = GameData::AmbientEndDimming.getPreciseTotalSeconds();

		// In Arena, the min ambient is 0 and the max ambient is 1, but we're using
		// some values here that make testing easier.
		const double minAmbient = 0.15;
		const double maxAmbient = 1.0;

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

void GameData::setTriggerText(const std::string &text, FontManager &fontManager, Renderer &renderer)
{
	const int lineSpacing = 1;
	const RichTextString richText(
		text,
		FontName::Arena,
		TriggerTextColor,
		TextAlignment::Center,
		lineSpacing,
		fontManager);

	const TextBox::ShadowData shadowData(TriggerTextShadowColor, Int2(-1, 0));

	// Create the text box for display (set position to zero; the renderer will
	// decide where to draw it).
	auto textBox = std::make_unique<TextBox>(
		Int2(0, 0),
		richText,
		&shadowData,
		renderer);

	// Assign the text box and its duration to the triggered text member.
	const double duration = std::max(2.50, static_cast<double>(text.size()) * 0.050);
	this->triggerText = TimedTextBox(duration, std::move(textBox));
}

void GameData::setActionText(const std::string &text, FontManager &fontManager, Renderer &renderer)
{
	const RichTextString richText(
		text,
		FontName::Arena,
		ActionTextColor,
		TextAlignment::Center,
		fontManager);

	const TextBox::ShadowData shadowData(ActionTextShadowColor, Int2(-1, 0));

	// Create the text box for display (set position to zero; the renderer will decide
	// where to draw it).
	auto textBox = std::make_unique<TextBox>(
		Int2(0, 0),
		richText,
		&shadowData,
		renderer);

	// Assign the text box and its duration to the action text.
	const double duration = std::max(2.25, static_cast<double>(text.size()) * 0.050);
	this->actionText = TimedTextBox(duration, std::move(textBox));
}

void GameData::setEffectText(const std::string &text, FontManager &fontManager, Renderer &renderer)
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

void GameData::tickTime(double dt, Game &game)
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
		const auto &exeData = game.getMiscAssets().getExeData();
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
	if (this->chasmAnimSeconds >= GameData::CHASM_ANIM_PERIOD)
	{
		this->chasmAnimSeconds = std::fmod(this->chasmAnimSeconds, GameData::CHASM_ANIM_PERIOD);
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
}
