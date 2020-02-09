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
#include "../Media/MusicFile.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../World/ClimateType.h"
#include "../World/ExteriorWorldData.h"
#include "../World/InteriorWorldData.h"
#include "../World/LocationType.h"
#include "../World/VoxelGrid.h"
#include "../World/WeatherType.h"
#include "../World/WorldType.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	// Arbitrary fog distances for each weather; the distance at which fog is maximum.
	const std::unordered_map<WeatherType, double> WeatherFogDistances =
	{
		{ WeatherType::Clear, 100.0 },
		{ WeatherType::Overcast, 30.0 },
		{ WeatherType::Rain, 50.0 },
		{ WeatherType::Snow, 25.0 },
		{ WeatherType::SnowOvercast, 20.0 },
		{ WeatherType::Rain2, 50.0 },
		{ WeatherType::Overcast2, 30.0 },
		{ WeatherType::SnowOvercast2, 20.0 }
	};

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

GameData::GameData(Player &&player, const MiscAssets &miscAssets)
	: player(std::move(player))
{
	// Most values need to be initialized elsewhere in the program in order to determine
	// the world state, etc..
	DebugLog("Initializing.");

	// Make a copy of the global constant city data. This is the "instance" city data
	// that can be assigned to.
	this->cityData = miscAssets.getCityDataFile();

	// Set default location visibilities.
	for (int i = 0; i < 8; i++)
	{
		auto &provinceData = this->cityData.getProvinceData(i);
		for (auto &cityState : provinceData.cityStates)
		{
			cityState.setVisible(true);
		}

		for (auto &town : provinceData.towns)
		{
			town.setVisible(true);
		}

		for (auto &village : provinceData.villages)
		{
			village.setVisible(true);
		}

		// Make main quest dungeons visible for testing.
		provinceData.firstDungeon.setVisible(true);
		provinceData.secondDungeon.setVisible(true);

		for (auto &dungeon : provinceData.randomDungeons)
		{
			dungeon.setVisible(false);
		}
	}

	auto &centerProvinceData = this->cityData.getProvinceData(8);
	centerProvinceData.cityStates.front().setVisible(true);

	// Do initial weather update (to set each value to a valid state).
	this->updateWeather(miscAssets.getExeData());

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

std::vector<uint32_t> GameData::makeExteriorSkyPalette(WeatherType weatherType,
	TextureManager &textureManager)
{
	// Get the palette name for the given weather.
	const std::string paletteName = (weatherType == WeatherType::Clear) ?
		"DAYTIME.COL" : "DREARY.COL";

	// The palettes in the data files only cover half of the day, so some added
	// darkness is needed for the other half.
	const Surface &palette = textureManager.getSurface(paletteName);
	const uint32_t *pixels = static_cast<const uint32_t*>(palette.getPixels());
	const int pixelCount = palette.getWidth() * palette.getHeight();

	// Fill palette with darkness (the first color in the palette is the closest to night).
	const uint32_t darkness = pixels[0];
	std::vector<uint32_t> fullPalette(pixelCount * 2, darkness);

	// Copy the sky palette over the center of the full palette.
	std::copy(pixels, pixels + pixelCount,
		fullPalette.data() + (fullPalette.size() / 4));

	return fullPalette;
}

double GameData::getFogDistanceFromWeather(WeatherType weatherType)
{
	return WeatherFogDistances.at(weatherType);
}

MusicName GameData::getExteriorMusicName(WeatherType weatherType)
{
	return MusicFile::fromWeather(weatherType);
}

MusicName GameData::getDungeonMusicName(Random &random)
{
	const std::array<MusicName, 5> DungeonMusics =
	{
		MusicName::Dungeon1,
		MusicName::Dungeon2,
		MusicName::Dungeon3,
		MusicName::Dungeon4,
		MusicName::Dungeon5
	};

	return DungeonMusics.at(random.next(static_cast<int>(DungeonMusics.size())));
}

MusicName GameData::getInteriorMusicName(const std::string &mifName, Random &random)
{
	// Check against all of the non-dungeon interiors first.
	const bool isEquipmentStore = mifName.find("EQUIP") != std::string::npos;
	const bool isHouse = (mifName.find("BS") != std::string::npos) ||
		(mifName.find("NOBLE") != std::string::npos);
	const bool isMagesGuild = mifName.find("MAGE") != std::string::npos;
	const bool isPalace = (mifName.find("PALACE") != std::string::npos) ||
		(mifName.find("TOWNPAL") != std::string::npos) ||
		(mifName.find("VILPAL") != std::string::npos);
	const bool isTavern = mifName.find("TAVERN") != std::string::npos;
	const bool isTemple = mifName.find("TEMPLE") != std::string::npos;

	if (isEquipmentStore)
	{
		return MusicName::Equipment;
	}
	else if (isHouse)
	{
		return MusicName::Sneaking;
	}
	else if (isMagesGuild)
	{
		return MusicName::Magic;
	}
	else if (isPalace)
	{
		return MusicName::Palace;
	}
	else if (isTavern)
	{
		const std::array<MusicName, 2> TavernMusics =
		{
			MusicName::Square,
			MusicName::Tavern
		};

		return TavernMusics.at(random.next(static_cast<int>(TavernMusics.size())));
	}
	else if (isTemple)
	{
		return MusicName::Temple;
	}
	else
	{
		// Dungeon.
		return GameData::getDungeonMusicName(random);
	}
}

void GameData::loadInterior(const MIFFile &mif, const Location &location,
	const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer)
{
	// Call interior WorldData loader.
	const auto &exeData = miscAssets.getExeData();
	this->worldData = std::make_unique<InteriorWorldData>(
		InteriorWorldData::loadInterior(mif, exeData));

	// Set initial level active in the renderer.
	LevelData &activeLevel = this->worldData->getActiveLevel();
	activeLevel.setActive(miscAssets, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = this->worldData->getStartPoints().front();
	this->player.teleport(Double3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Set location.
	this->location = location;

	// Arbitrary interior weather and fog.
	const double fogDistance = GameData::DEFAULT_INTERIOR_FOG_DIST;
	this->weatherType = WeatherType::Clear;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
}

void GameData::enterInterior(const MIFFile &mif, const Int2 &returnVoxel,
	const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssert(this->worldData.get() != nullptr);
	DebugAssert(this->worldData->getActiveWorldType() != WorldType::Interior);

	ExteriorWorldData &exterior = static_cast<ExteriorWorldData&>(*this->worldData.get());
	DebugAssert(exterior.getInterior() == nullptr);

	const auto &exeData = miscAssets.getExeData();
	InteriorWorldData interior = InteriorWorldData::loadInterior(mif, exeData);

	// Give the interior world data to the active exterior.
	exterior.enterInterior(std::move(interior), returnVoxel);

	// Set interior level active in the renderer.
	LevelData &activeLevel = exterior.getActiveLevel();
	activeLevel.setActive(miscAssets, textureManager, renderer);

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
	activeLevel.setActive(miscAssets, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 startPoint(
		static_cast<double>(returnVoxel.x) + 0.50,
		static_cast<double>(returnVoxel.y) + 0.50);
	this->player.teleport(Double3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Regular sky palette based on weather.
	const std::vector<uint32_t> skyPalette =
		GameData::makeExteriorSkyPalette(this->weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.data(), static_cast<int>(skyPalette.size()));

	// Set fog and night lights.
	const double fogDistance = GameData::getFogDistanceFromWeather(this->weatherType);
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
	renderer.setNightLightsActive(this->clock.nightLightsAreActive());
}

void GameData::loadNamedDungeon(int localDungeonID, int provinceID, bool isArtifactDungeon,
	const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer)
{
	// Dungeon ID must be for a named dungeon, not main quest dungeon.
	DebugAssertMsg(localDungeonID >= 2, "Dungeon ID \"" + std::to_string(localDungeonID) +
		"\" must not be for main quest dungeon.");

	// Generate dungeon seed.
	const uint32_t dungeonSeed = this->cityData.getDungeonSeed(localDungeonID, provinceID);

	// Call dungeon WorldData loader with parameters specific to named dungeons.
	const auto &exeData = miscAssets.getExeData();
	const int widthChunks = 2;
	const int depthChunks = 1;
	this->worldData = std::make_unique<InteriorWorldData>(InteriorWorldData::loadDungeon(
		dungeonSeed, widthChunks, depthChunks, isArtifactDungeon, exeData));

	// Set initial level active in the renderer.
	LevelData &activeLevel = this->worldData->getActiveLevel();
	activeLevel.setActive(miscAssets, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = this->worldData->getStartPoints().front();
	this->player.teleport(Double3(
		startPoint.x - 1.0, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Set location.
	this->location = Location::makeDungeon(localDungeonID, provinceID);

	// Arbitrary interior weather and fog.
	const double fogDistance = GameData::DEFAULT_INTERIOR_FOG_DIST;
	this->weatherType = WeatherType::Clear;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
}

void GameData::loadWildernessDungeon(int provinceID, int wildBlockX, int wildBlockY,
	const CityDataFile &cityData, const MiscAssets &miscAssets, TextureManager &textureManager,
	Renderer &renderer)
{
	// Verify that the wilderness block coordinates are valid (0..63).
	DebugAssertMsg((wildBlockX >= 0) && (wildBlockX < RMDFile::WIDTH),
		"Wild block X \"" + std::to_string(wildBlockX) + "\" out of range.");
	DebugAssertMsg((wildBlockY >= 0) && (wildBlockY < RMDFile::DEPTH),
		"Wild block Y \"" + std::to_string(wildBlockY) + "\" out of range.");

	// Generate wilderness dungeon seed.
	const uint32_t wildDungeonSeed = cityData.getWildernessDungeonSeed(
		provinceID, wildBlockX, wildBlockY);

	// Call dungeon WorldData loader with parameters specific to wilderness dungeons.
	const auto &exeData = miscAssets.getExeData();
	const int widthChunks = 2;
	const int depthChunks = 2;
	const bool isArtifactDungeon = false;
	this->worldData = std::make_unique<InteriorWorldData>(InteriorWorldData::loadDungeon(
		wildDungeonSeed, widthChunks, depthChunks, isArtifactDungeon, exeData));

	// Set initial level active in the renderer.
	LevelData &activeLevel = this->worldData->getActiveLevel();
	activeLevel.setActive(miscAssets, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = this->worldData->getStartPoints().front();
	this->player.teleport(Double3(
		startPoint.x - 1.0, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Set location (since wilderness dungeons aren't their own location, use a placeholder
	// value for testing).
	this->location = Location::makeSpecialCase(Location::SpecialCaseType::WildDungeon, provinceID);

	// Arbitrary interior weather and fog.
	const double fogDistance = GameData::DEFAULT_INTERIOR_FOG_DIST;
	this->weatherType = WeatherType::Clear;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
}

void GameData::loadPremadeCity(const MIFFile &mif, WeatherType weatherType, int starCount,
	const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer)
{
	// Climate for center province.
	const int localCityID = 0;
	const int provinceID = Location::CENTER_PROVINCE_ID;
	const ClimateType climateType = Location::getCityClimateType(
		localCityID, provinceID, miscAssets);

	// Call premade city loader.
	this->worldData = std::make_unique<ExteriorWorldData>(ExteriorWorldData::loadPremadeCity(
		mif, climateType, weatherType, this->date.getDay(), starCount,
		miscAssets, textureManager));

	// Set initial level active in the renderer.
	LevelData &activeLevel = this->worldData->getActiveLevel();
	activeLevel.setActive(miscAssets, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = this->worldData->getStartPoints().front();
	this->player.teleport(Double3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Set location.
	this->location = Location::makeCity(localCityID, provinceID);

	// Regular sky palette based on weather.
	const std::vector<uint32_t> skyPalette =
		GameData::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.data(), static_cast<int>(skyPalette.size()));

	// Set weather, fog, and night lights.
	const double fogDistance = GameData::getFogDistanceFromWeather(weatherType);
	this->weatherType = weatherType;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
	renderer.setNightLightsActive(this->clock.nightLightsAreActive());
}

void GameData::loadCity(int localCityID, int provinceID, WeatherType weatherType, int starCount,
	const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer)
{
	const int globalCityID = CityDataFile::getGlobalCityID(localCityID, provinceID);

	// Check that the IDs are in the proper range. Although 256 is a valid city ID,
	// loadPremadeCity() should be called instead for that case.
	DebugAssertMsg(provinceID != Location::CENTER_PROVINCE_ID,
		"Use loadPremadeCity() instead for center province.");
	DebugAssertMsg((globalCityID >= 0) && (globalCityID < 256),
		"Invalid city ID \"" + std::to_string(globalCityID) + "\".");

	// Determine city traits from the given city ID.
	const LocationType locationType = Location::getCityType(localCityID);
	const ExeData::CityGeneration &cityGen = miscAssets.getExeData().cityGen;
	const bool isCityState = locationType == LocationType::CityState;
	const bool isCoastal = std::find(cityGen.coastalCityList.begin(),
		cityGen.coastalCityList.end(), globalCityID) != cityGen.coastalCityList.end();
	const int templateCount = CityDataFile::getCityTemplateCount(isCoastal, isCityState);
	const int templateID = globalCityID % templateCount;

	const std::string mifName = [locationType, &cityGen, isCoastal, templateID]()
	{
		// Get the index into the template names array (town%d.mif, ..., cityw%d.mif).
		const int nameIndex = CityDataFile::getCityTemplateNameIndex(locationType, isCoastal);

		// Get the template name associated with the city ID.
		std::string templateName = cityGen.templateFilenames.at(nameIndex);
		templateName = String::replace(templateName, "%d", std::to_string(templateID + 1));
		templateName = String::toUppercase(templateName);

		return templateName;
	}();

	MIFFile mif;
	if (!mif.init(mifName.c_str()))
	{
		DebugCrash("Could not init .MIF file \"" + mifName + "\".");
	}

	// City block count (6x6, 5x5, 4x4).
	const int cityDim = CityDataFile::getCityDimensions(locationType);

	// Get the reserved block list for the given city.
	const std::vector<uint8_t> &reservedBlocks = [&cityGen, isCoastal, templateID]()
	{
		const int index = CityDataFile::getCityReservedBlockListIndex(isCoastal, templateID);
		return cityGen.reservedBlockLists.at(index);
	}();

	// Get the starting position of city blocks within the city skeleton.
	const Int2 startPosition = [locationType, &cityGen, isCoastal, templateID]()
	{
		const int index = CityDataFile::getCityStartingPositionIndex(
			locationType, isCoastal, templateID);

		const auto &pair = cityGen.startingPositions.at(index);
		return Int2(pair.first, pair.second);
	}();

	// Call city WorldData loader.
	this->worldData = std::make_unique<ExteriorWorldData>(ExteriorWorldData::loadCity(
		localCityID, provinceID, mif, cityDim, isCoastal, reservedBlocks, startPosition,
		weatherType, this->date.getDay(), starCount, miscAssets, textureManager));

	// Set initial level active in the renderer.
	LevelData &activeLevel = this->worldData->getActiveLevel();
	activeLevel.setActive(miscAssets, textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = this->worldData->getStartPoints().front();
	this->player.teleport(Double3(
		startPoint.x, activeLevel.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Set location.
	this->location = Location::makeCity(localCityID, provinceID);

	// Regular sky palette based on weather.
	const std::vector<uint32_t> skyPalette =
		GameData::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.data(), static_cast<int>(skyPalette.size()));

	// Set weather, fog, and night lights.
	const double fogDistance = GameData::getFogDistanceFromWeather(weatherType);
	this->weatherType = weatherType;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
	renderer.setNightLightsActive(this->clock.nightLightsAreActive());
}

void GameData::loadWilderness(int localCityID, int provinceID, const Int2 &gatePos,
	const Int2 &transitionDir, bool debug_ignoreGatePos, WeatherType weatherType,
	int starCount, const MiscAssets &miscAssets, TextureManager &textureManager,
	Renderer &renderer)
{
	// Call wilderness WorldData loader.
	this->worldData = std::make_unique<ExteriorWorldData>(ExteriorWorldData::loadWilderness(
		localCityID, provinceID, weatherType, this->date.getDay(), starCount,
		miscAssets, textureManager));

	// Set initial level active in the renderer.
	LevelData &activeLevel = this->worldData->getActiveLevel();
	activeLevel.setActive(miscAssets, textureManager, renderer);

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

	// Set location.
	this->location = Location::makeCity(localCityID, provinceID);

	// Regular sky palette based on weather.
	const std::vector<uint32_t> skyPalette =
		GameData::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.data(), static_cast<int>(skyPalette.size()));

	// Set weather, fog, and night lights.
	const double fogDistance = GameData::getFogDistanceFromWeather(weatherType);
	this->weatherType = weatherType;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
	renderer.setNightLightsActive(this->clock.nightLightsAreActive());
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

Location &GameData::getLocation()
{
	return this->location;
}

CityDataFile &GameData::getCityDataFile()
{
	return this->cityData;
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

WeatherType GameData::getFilteredWeatherType(WeatherType weatherType,
	ClimateType climateType)
{
	// Snow in deserts is replaced by rain.
	const bool isSnow = (weatherType == WeatherType::Snow) ||
		(weatherType == WeatherType::SnowOvercast) ||
		(weatherType == WeatherType::SnowOvercast2);
	return ((climateType == ClimateType::Desert) && isSnow) ?
		WeatherType::Rain : weatherType;
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
		const double startBrighteningTime =
			Clock::AmbientStartBrightening.getPreciseTotalSeconds();
		const double endBrighteningTime =
			Clock::AmbientEndBrightening.getPreciseTotalSeconds();
		const double startDimmingTime =
			Clock::AmbientStartDimming.getPreciseTotalSeconds();
		const double endDimmingTime =
			Clock::AmbientEndDimming.getPreciseTotalSeconds();

		// In Arena, the min ambient is 0 and the max ambient is 1, but we're using
		// some values here that make testing easier.
		const double minAmbient = 0.30;
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
